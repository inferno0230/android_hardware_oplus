package com.oplus.wrapper.hardware.camera2.impl;

public class CameraMetadataNative {

    private final android.hardware.camera2.impl.CameraMetadataNative mCameraMetadataNative;

    public CameraMetadataNative(android.hardware.camera2.impl.CameraMetadataNative metadata) {
        mCameraMetadataNative = metadata;
    }

    public long getMetadataPtr() {
        return mCameraMetadataNative.getMetadataPtr();
    }
}
