package com.oplus.wrapper.hardware.display;

public class WifiDisplayStatus {

    private final android.hardware.display.WifiDisplayStatus mWifiDisplayStatus;

    public WifiDisplayStatus(android.hardware.display.WifiDisplayStatus wifiDisplayStatus) {
        mWifiDisplayStatus = wifiDisplayStatus;
    }

    public int getActiveDisplayState() {
        return mWifiDisplayStatus.getActiveDisplayState();
    }
}
