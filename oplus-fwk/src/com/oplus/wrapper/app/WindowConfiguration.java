package com.oplus.wrapper.app;

import android.graphics.Rect;

public class WindowConfiguration {

    private final android.app.WindowConfiguration mWindowConfiguration;

    public WindowConfiguration(android.app.WindowConfiguration windowConfiguration) {
        mWindowConfiguration = windowConfiguration;
    }

    public Rect getAppBounds() {
        return mWindowConfiguration.getAppBounds();
    }

    public Rect getBounds() {
        return mWindowConfiguration.getBounds();
    }
}
