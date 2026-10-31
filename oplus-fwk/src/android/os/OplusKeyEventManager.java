package android.os;

import android.util.ArrayMap;

import android.content.Context;

import android.view.KeyEvent;

public class OplusKeyEventManager {

    public interface OnKeyEventObserver {
        void onKeyEvent(KeyEvent event); 
    }

    private static OplusKeyEventManager sInstance = null;

    public static OplusKeyEventManager getInstance() {
        if (sInstance == null) {
            sInstance = new OplusKeyEventManager();
        }
        return sInstance;
    }

    public boolean registerKeyEventObserver(Context context, OnKeyEventObserver observer, int listenFlag) {
        return false;
    }

    public boolean unregisterKeyEventObserver(Context context, OnKeyEventObserver observer) {
        return false;
    }

    public boolean registerKeyEventInterceptor(Context context, String description,
            OnKeyEventObserver observer, ArrayMap<Integer, Integer> configs) {
        return false;
    }

    public boolean unregisterKeyEventInterceptor(Context context, String description,
            OnKeyEventObserver observer) {
        return false;
    }
}
