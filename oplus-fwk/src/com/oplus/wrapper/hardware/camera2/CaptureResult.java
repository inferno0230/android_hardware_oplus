package com.oplus.wrapper.hardware.camera2;

import com.oplus.wrapper.hardware.camera2.impl.CameraMetadataNative;

public class CaptureResult {

    private final android.hardware.camera2.CaptureResult mCaptureResult;

    public CaptureResult(android.hardware.camera2.CaptureResult captureResult) {
        mCaptureResult = captureResult;
    }

    public CameraMetadataNative getNativeMetadata() {
        android.hardware.camera2.impl.CameraMetadataNative metadata =
                mCaptureResult.getNativeMetadata();
        return metadata != null ? new CameraMetadataNative(metadata) : null;
    }
}
