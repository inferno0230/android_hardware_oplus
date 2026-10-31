/*
 * Copyright (C) 2022 The Nameless-AOSP Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.oplus.os;

import android.content.Context;
import android.os.IBinder;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;

/**
 * AOSP-backed stand-in for the OPlus {@code com.oplus.os.LinearmotorVibrator} system service.
 *
 * <p>The OPlus camera app talks to the OPlus linear-motor vibrator through
 * {@code Context.getSystemService("linearmotor")}. That service only exists in a stock OPlus
 * framework, so on AOSP the lookup returns {@code null} and all camera haptics are silently
 * dropped.
 *
 * <p>This class is shipped on the boot classpath by {@code oplus-fwk} and provides the same API
 * surface, but drives the standard AOSP {@link Vibrator}/{@link VibratorManager} instead. The
 * camera app is patched (see {@code vendor/oplus/camera/<device>/patches}) to obtain an instance
 * through {@link #create(Context)} rather than the missing OPlus system service.
 *
 * <p>The device's vibrator HAL (vendor.qti.hardware.vibrator) already carries the stock OPlus
 * richtap effect streams, so the predefined AOSP effects used here produce stock-quality haptics.
 */
public class LinearmotorVibrator {
    private static final String TAG = "LinearmotorVibrator";

    private final Context mContext;
    private final Vibrator mVibrator;

    public LinearmotorVibrator(Context context, ILinearmotorVibratorService serviceIgnored) {
        mContext = context;

        Vibrator vibrator = null;
        if (context != null) {
            VibratorManager manager =
                    (VibratorManager) context.getSystemService(Context.VIBRATOR_MANAGER_SERVICE);
            if (manager != null) {
                vibrator = manager.getDefaultVibrator();
            }
            if (vibrator == null) {
                vibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
            }
        }
        mVibrator = vibrator;
    }

    /**
     * Factory used by the patched OPlus camera app in place of
     * {@code Context.getSystemService(Context.LINEARMOTOR_VIBRATOR_SERVICE)}.
     */
    public static LinearmotorVibrator create(Context context) {
        return new LinearmotorVibrator(context, null);
    }

    private static VibrationEffect toVibrationEffect(WaveformEffect effect) {
        // The OPlus WaveformEffect carries a proprietary effect type that only the OPlus
        // vibrator service understands. Map it onto the closest AOSP predefined effect; the
        // QTI vibrator HAL resolves these through the extracted stock OPlus richtap streams.
        // EFFECT_CLICK is the crispest match for the camera shutter / UI feedback.
        return VibrationEffect.createPredefined(VibrationEffect.EFFECT_CLICK);
    }

    public void vibrate(WaveformEffect effect) {
        if (mVibrator == null) {
            return;
        }
        // Plain Vibrator.vibrate() uses VibrationAttributes.USAGE_UNKNOWN, which the framework
        // resolves to the "media" vibration intensity. That is deliberate: like stock OPlus, the
        // camera shutter must still buzz even when the user has turned touch haptics off. Switch
        // to VibrationAttributes.createForUsage(USAGE_TOUCH) if it should follow the
        // touch-feedback slider instead.
        mVibrator.vibrate(toVibrationEffect(effect));
    }

    public void cancelVibrate(WaveformEffect effect) {
        if (mVibrator != null) {
            mVibrator.cancel();
        }
    }

    public int getVibratorStatus() {
        return 0; // always ready
    }

    public void setVibratorStrength(int strength) {
        // no-op
    }

    public int getSettingsTouchEffectStrength() {
        return 2400; // default max
    }

    public int getSettingsRingtoneEffectStrength() {
        return 2400;
    }

    public int getSettingsNotificationEffectStrength() {
        return 2400;
    }

    public int getVibratorTouchStyle() {
        return 0;
    }

    public void setVibratorTouchStyle(int style) {
        // no-op
    }

    public void updateVibrationAmplitude(float amplitudeRatio) {
        // no-op
    }

    public VibrationEffect createWaveformEffect(int effectId, int strength, int repeat) {
        return toVibrationEffect(null);
    }

    public VibrationEffect createWaveformEffect(String ringtonePath, int strength, int repeat) {
        return toVibrationEffect(null);
    }

    public void vibrate(VibrationEffect effect, android.os.VibrationAttributes attributes) {
        if (mVibrator == null) {
            return;
        }
        mVibrator.vibrate(toVibrationEffect(null));
    }

    public void vibrate(VibrationEffect effect, String reason,
            android.os.VibrationAttributes attributes) {
        if (mVibrator == null) {
            return;
        }
        mVibrator.vibrate(toVibrationEffect(null));
    }

    public void vibrate(int uid, String opPkg, VibrationEffect effect, String reason,
            android.os.VibrationAttributes attributes) {
        if (mVibrator == null) {
            return;
        }
        mVibrator.vibrate(toVibrationEffect(null));
    }

    /*
     * The remaining OPlus service entry points, kept so this class matches
     * com.oplus.os.ILinearmotorVibratorService one for one. Nothing on the device calls them
     * today; they all resolve to the AOSP-backed implementations above.
     */

    public void vibrate(int uid, String opPkg, WaveformEffect effect, IBinder token) {
        vibrate(effect);
    }

    public void cancelVibrate(WaveformEffect effect, IBinder token) {
        cancelVibrate(effect);
    }

    public void updateVibrationAmplitude(int uid, String opPkg, float amplitudeRatio) {
        updateVibrationAmplitude(amplitudeRatio);
    }

    public boolean checkRichtapBlackList(String packageName) {
        // No richtap blacklist on AOSP: nothing is blocked.
        return false;
    }
}
