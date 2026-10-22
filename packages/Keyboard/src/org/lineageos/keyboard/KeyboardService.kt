/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package org.lineageos.keyboard

import android.app.Service
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothHidHost
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.input.InputManager
import android.os.Handler
import android.os.HandlerThread
import android.os.IBinder
import android.os.UEventObserver
import android.util.Log
import android.view.InputDevice

class KeyboardService : Service() {
    private val bluetoothManager by lazy { getSystemService(BluetoothManager::class.java) }
    private val inputManager by lazy { getSystemService(InputManager::class.java) }
    private val sharedPreferences by lazy { getSharedPreferences(PREFS_NAME, MODE_PRIVATE) }

    private val thread = HandlerThread(TAG)
    private val handler by lazy { Handler(thread.looper) }

    private var hidHost: BluetoothHidHost? = null
    private var scanCallback: ScanCallback? = null

    private var keyboardAddr: String? = null
    private var isAttached: Boolean? = null

    private val stopScanRunnable = Runnable { stopScan() }

    private val observer =
        object : UEventObserver() {
            override fun onUEvent(event: UEvent) {
                val isAttached =
                    when (event.get("pogopin_status")) {
                        "0" -> false
                        "1" -> true
                        else -> return
                    }
                // The address is all zeroes until the keyboard reported it over the pogo pins
                val keyboardAddr =
                    event
                        .get("mac_addr")
                        ?.takeIf { it.length == 12 && it.any { c -> c != '0' } }
                        ?.chunked(2)
                        ?.joinToString(":") { it.uppercase() }

                handler.post { onPogoStatusChanged(isAttached, keyboardAddr) }
            }
        }

    private val profileListener =
        object : BluetoothProfile.ServiceListener {
            override fun onServiceConnected(profile: Int, proxy: BluetoothProfile) {
                handler.post {
                    hidHost = proxy as BluetoothHidHost
                    updateConnection()
                }
            }

            override fun onServiceDisconnected(profile: Int) {
                handler.post { hidHost = null }
            }
        }

    private val receiver =
        object : BroadcastReceiver() {
            override fun onReceive(context: Context, intent: Intent) {
                when (intent.action) {
                    BluetoothAdapter.ACTION_STATE_CHANGED -> {
                        when (intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, -1)) {
                            BluetoothAdapter.STATE_ON -> updateConnection()
                            // Scan callbacks stop once Bluetooth goes down
                            BluetoothAdapter.STATE_TURNING_OFF -> stopScan()
                        }
                    }
                    BluetoothDevice.ACTION_BOND_STATE_CHANGED -> {
                        if (intent.device?.address != keyboardAddr) {
                            return
                        }
                        val state = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, -1)
                        if (state == BluetoothDevice.BOND_BONDED) {
                            updateConnection()
                        }
                    }
                }
            }
        }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()

        thread.start()
        keyboardAddr = sharedPreferences.getString(KEY_KEYBOARD_ADDR, null)

        registerReceiver(
            receiver,
            IntentFilter().apply {
                addAction(BluetoothAdapter.ACTION_STATE_CHANGED)
                addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED)
            },
            null,
            handler,
            Context.RECEIVER_EXPORTED,
        )
        bluetoothManager.adapter?.getProfileProxy(this, profileListener, BluetoothProfile.HID_HOST)

        observer.startObserving("DEVPATH=/devices/virtual/pogopin/pogo_keyboard")

        handler.post {
            if (isAttached == null) {
                isAttached = isPogoKeyboardPresent()
                updateConnection()
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()

        observer.stopObserving()
        unregisterReceiver(receiver)

        handler.post {
            stopScan()
            hidHost?.let {
                bluetoothManager.adapter?.closeProfileProxy(BluetoothProfile.HID_HOST, it)
            }
        }
        thread.quitSafely()
    }

    private fun onPogoStatusChanged(isAttached: Boolean, keyboardAddr: String?) {
        if (keyboardAddr != null && keyboardAddr != this.keyboardAddr) {
            Log.i(TAG, "Keyboard address changed to $keyboardAddr")
            this.keyboardAddr = keyboardAddr
            sharedPreferences.edit().putString(KEY_KEYBOARD_ADDR, keyboardAddr).apply()
        } else if (isAttached == this.isAttached) {
            return
        }
        Log.i(TAG, "Keyboard ${if (isAttached) "attached" else "detached"}")
        this.isAttached = isAttached
        updateConnection()
    }

    // Only use Bluetooth while the keyboard is off the pogo pins, or every key is reported twice
    private fun updateConnection() {
        val keyboardAddr = keyboardAddr ?: return
        val isAttached = isAttached ?: return
        val adapter = bluetoothManager.adapter?.takeIf { it.isEnabled } ?: return
        val device = adapter.getRemoteDevice(keyboardAddr)

        if (isAttached) {
            stopScan()
            if (device.bondState == BluetoothDevice.BOND_BONDED) {
                // This also disconnects it
                hidHost?.setConnectionPolicy(device, BluetoothProfile.CONNECTION_POLICY_FORBIDDEN)
            }
            return
        }

        when (device.bondState) {
            BluetoothDevice.BOND_BONDED -> {
                // This also connects it
                hidHost?.setConnectionPolicy(device, BluetoothProfile.CONNECTION_POLICY_ALLOWED)
            }
            BluetoothDevice.BOND_NONE -> bondBtDevice(adapter, keyboardAddr)
        }
    }

    // The keyboard only advertises once it's off the pogo pins, which can take a while after a
    // long time attached
    private fun bondBtDevice(adapter: BluetoothAdapter, keyboardAddr: String) {
        if (scanCallback != null) {
            return
        }
        val scanner = adapter.bluetoothLeScanner ?: return
        val callback =
            object : ScanCallback() {
                override fun onScanResult(callbackType: Int, result: ScanResult) {
                    handler.post {
                        if (scanCallback !== this) {
                            return@post
                        }
                        stopScan()
                        // The keyboard is LE only, don't let the stack fall back to BR/EDR
                        result.device.createBond(BluetoothDevice.TRANSPORT_LE)
                    }
                }

                override fun onScanFailed(errorCode: Int) {
                    Log.e(TAG, "Scan failed: $errorCode")
                    handler.post {
                        if (scanCallback === this) {
                            stopScan()
                        }
                    }
                }
            }
        scanCallback = callback
        scanner.startScan(
            listOf(ScanFilter.Builder().setDeviceAddress(keyboardAddr).build()),
            ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                .setReportDelay(0L)
                .build(),
            callback,
        )
        handler.postDelayed(stopScanRunnable, SCAN_TIMEOUT_MS)
    }

    private fun stopScan() {
        handler.removeCallbacks(stopScanRunnable)
        val callback = scanCallback ?: return
        scanCallback = null
        bluetoothManager.adapter?.bluetoothLeScanner?.stopScan(callback)
    }

    private fun isPogoKeyboardPresent() =
        inputManager.inputDeviceIds.any {
            val device = inputManager.getInputDevice(it) ?: return@any false
            // The pogo driver registers the keyboard with the vendor ID of the Bluetooth one
            device.vendorId == VENDOR_ID_ONEPLUS &&
                device.bluetoothAddress == null &&
                device.keyboardType == InputDevice.KEYBOARD_TYPE_ALPHABETIC
        }

    private val Intent.device
        get() = getParcelableExtra(BluetoothDevice.EXTRA_DEVICE, BluetoothDevice::class.java)

    companion object {
        private const val TAG = "OplusKeyboardService"

        private const val PREFS_NAME = "keyboard"
        private const val KEY_KEYBOARD_ADDR = "keyboard_addr"

        private const val VENDOR_ID_ONEPLUS = 0x22D9

        private const val SCAN_TIMEOUT_MS = 60_000L
    }
}
