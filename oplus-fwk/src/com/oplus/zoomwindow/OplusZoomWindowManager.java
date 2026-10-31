package com.oplus.zoomwindow;

import android.os.Bundle;

import android.content.Intent;

public class OplusZoomWindowManager {

    public static OplusZoomWindowManager sOplusZoomWindowManager = null;

    public static OplusZoomWindowManager getInstance() {
        if (sOplusZoomWindowManager == null) {
            sOplusZoomWindowManager = new OplusZoomWindowManager();
        }
        return sOplusZoomWindowManager;
    }

    public boolean registerZoomWindowObserver(IOplusZoomWindowObserver observer) {
        return false;
    }

    public boolean unregisterZoomWindowObserver(IOplusZoomWindowObserver observer) {
        return false;
    }

    public OplusZoomWindowInfo getCurrentZoomWindowState() {
        return new OplusZoomWindowInfo();
    }

    public boolean isSupportZoomWindowMode() {
        return false;
    }

    public boolean isSupportZoomMode(String pkg, int userId, String callPkg, Bundle extension) {
        return false;
    }

    public int startZoomWindow(Intent intent, Bundle bOptions, int userId, String callPkg) {
        return -1;
    }
}
