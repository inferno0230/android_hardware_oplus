package com.oplus.wrapper.view;

public class WindowManagerGlobal {

    private static WindowManagerGlobal sInstance;

    private final android.view.WindowManagerGlobal mWindowManagerGlobal;

    private WindowManagerGlobal() {
        mWindowManagerGlobal = android.view.WindowManagerGlobal.getInstance();
    }

    public static synchronized WindowManagerGlobal getInstance() {
        if (sInstance == null) {
            sInstance = new WindowManagerGlobal();
        }
        return sInstance;
    }

    public void trimMemory(int level) {
        mWindowManagerGlobal.trimMemory(level);
    }
}
