package com.oplus.wrapper.hardware.display;

public class DisplayManager {

    private final android.hardware.display.DisplayManager mDisplayManager;

    public DisplayManager(android.hardware.display.DisplayManager displayManager) {
        mDisplayManager = displayManager;
    }

    public WifiDisplayStatus getWifiDisplayStatus() {
        android.hardware.display.WifiDisplayStatus status = mDisplayManager.getWifiDisplayStatus();
        return status != null ? new WifiDisplayStatus(status) : null;
    }
}
