package com.oplus.wrapper.widget;

import android.view.WindowManager;

public class Toast {

    private final android.widget.Toast mToast;

    public Toast(android.widget.Toast toast) {
        mToast = toast;
    }

    public WindowManager.LayoutParams getWindowParams() {
        return mToast.getWindowParams();
    }
}
