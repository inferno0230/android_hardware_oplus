package com.oplus.wrapper.view;

import android.view.SurfaceControl;

public class DragEvent {

    private final android.view.DragEvent mDragEvent;

    public DragEvent(android.view.DragEvent dragEvent) {
        mDragEvent = dragEvent;
    }

    public SurfaceControl getDragSurface() {
        return mDragEvent.getDragSurface();
    }
}
