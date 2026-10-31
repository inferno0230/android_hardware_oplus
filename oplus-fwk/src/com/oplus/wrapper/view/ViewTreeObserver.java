package com.oplus.wrapper.view;

import android.graphics.Region;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class ViewTreeObserver {

    private final android.view.ViewTreeObserver mViewTreeObserver;
    private final Map<OnComputeInternalInsetsListener,
            android.view.ViewTreeObserver.OnComputeInternalInsetsListener> mListeners =
            new ConcurrentHashMap<>();

    public interface OnComputeInternalInsetsListener {
        void onComputeInternalInsets(InternalInsetsInfo info);
    }

    public ViewTreeObserver(android.view.ViewTreeObserver viewTreeObserver) {
        mViewTreeObserver = viewTreeObserver;
    }

    public void addOnComputeInternalInsetsListener(OnComputeInternalInsetsListener listener) {
        if (listener == null) {
            return;
        }
        android.view.ViewTreeObserver.OnComputeInternalInsetsListener proxy =
                mListeners.computeIfAbsent(listener, l ->
                        info -> l.onComputeInternalInsets(new InternalInsetsInfo(info)));
        mViewTreeObserver.addOnComputeInternalInsetsListener(proxy);
    }

    public void removeOnComputeInternalInsetsListener(OnComputeInternalInsetsListener listener) {
        if (listener == null) {
            return;
        }
        android.view.ViewTreeObserver.OnComputeInternalInsetsListener proxy =
                mListeners.remove(listener);
        if (proxy != null) {
            mViewTreeObserver.removeOnComputeInternalInsetsListener(proxy);
        }
    }

    public static final class InternalInsetsInfo {

        public static final int TOUCHABLE_INSETS_FRAME =
                android.view.ViewTreeObserver.InternalInsetsInfo.TOUCHABLE_INSETS_FRAME;
        public static final int TOUCHABLE_INSETS_CONTENT =
                android.view.ViewTreeObserver.InternalInsetsInfo.TOUCHABLE_INSETS_CONTENT;
        public static final int TOUCHABLE_INSETS_VISIBLE =
                android.view.ViewTreeObserver.InternalInsetsInfo.TOUCHABLE_INSETS_VISIBLE;
        public static final int TOUCHABLE_INSETS_REGION =
                android.view.ViewTreeObserver.InternalInsetsInfo.TOUCHABLE_INSETS_REGION;

        private final android.view.ViewTreeObserver.InternalInsetsInfo mInfo;

        InternalInsetsInfo(android.view.ViewTreeObserver.InternalInsetsInfo info) {
            mInfo = info;
        }

        public void setTouchableInsets(int val) {
            mInfo.setTouchableInsets(val);
        }

        public Region getTouchableRegion() {
            return mInfo.touchableRegion;
        }
    }
}
