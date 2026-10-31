/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Reconstructed from OOS 16.0.8 oplus-framework.jar (com.oplus.media.OplusHeifWriter)
 * to restore the OEM HEIF/live-photo writer JNI shim that the LineageOS port dropped.
 * Native side: /system_ext/lib64/liboplusheifwriter.so (register_android_graphic_HeifWriter).
 * Method names, native signatures and field names MUST match the .so's JNI bindings exactly
 * (including the original "Destory"/"mcro" spellings).
 */

package com.oplus.media;

import android.graphics.Bitmap;
import android.util.Log;

import java.io.FileDescriptor;

public class OplusHeifWriter {
    private static final String TAG = "OplusHeifWriter_Java";

    public static final int COLOR_FMT_YUV420Planar = 0;
    public static final int COLOR_FMT_P010 = 1;
    public static final int COLOR_FMT_RGBA8888 = 2;
    public static final int COLOR_FMT_NV12 = 3;
    public static final int COLOR_FMT_NV21 = 4;
    public static final int COLOR_FMT_MAX = 5;

    static final int maxValue = 100;
    static final int minValue = 0;

    private long mNativeObject;

    static {
        Log.v(TAG, "loadLibrary");
        System.loadLibrary("oplusheifwriter");
    }

    public OplusHeifWriter() {
        mNativeObject = nativeSetup();
    }

    private static native long nativeSetup();

    private static native long nativeCreate(long nativeObject, int width, int height,
            int strideWidth, int strideHeight, int fmt, int quality, int rotation);

    private static native long nativeCreateLivePhoto(long nativeObject, byte[] yuvBuffer,
            byte[] exifData, FileDescriptor outFd, Options opts);

    private static native long nativeCreateLivePhotoByBmp(long nativeObject, Bitmap bmp,
            byte[] exifData, FileDescriptor outFd, Options opts);

    private static native long nativeProcessHeicPhotoFrame(long nativeObject, byte[] yuvBuffer,
            byte[] exifData, FileDescriptor fd);

    private static native void nativeDestory(long nativeObject);

    public boolean createPrimaryImage(int width, int height, int strideWidth, int strideHeight,
            int fmt, int quality, int rotation) {
        if (quality <= 0 || quality > 100) {
            throw new IllegalArgumentException("quality range error");
        }
        if (width <= 0 || height <= 0 || strideWidth <= 0 || strideHeight <= 0
                || fmt < 0 || fmt >= COLOR_FMT_MAX) {
            Log.i(TAG, "Input param error.");
            return false;
        }
        long ret = nativeCreate(mNativeObject, width, height, strideWidth, strideHeight,
                fmt, quality, rotation);
        Log.i(TAG, " OplusHeifWriter start! quality: " + quality);
        if (ret < 0) {
            return false;
        }
        return true;
    }

    public boolean processPrimaryImage(byte[] yuvBuffer, byte[] exifData, FileDescriptor fd) {
        long ret = nativeProcessHeicPhotoFrame(mNativeObject, yuvBuffer, exifData, fd);
        if (ret < 0) {
            return false;
        }
        return true;
    }

    public boolean processPrimaryLivePhoto(byte[] yuvBuffer, byte[] exifData, FileDescriptor outFd,
            Options opts) {
        long ret = nativeCreateLivePhoto(mNativeObject, yuvBuffer, exifData, outFd, opts);
        if (ret < 0) {
            Log.i(TAG, "processPrimaryLivePhoto failed!");
            return false;
        }
        return true;
    }

    public boolean processPrimaryLivePhoto(Bitmap bmp, byte[] exifData, FileDescriptor outFd,
            Options opts) {
        long ret = nativeCreateLivePhotoByBmp(mNativeObject, bmp, exifData, outFd, opts);
        if (ret < 0) {
            Log.i(TAG, "processPrimaryLivePhoto failed!");
            return false;
        }
        return true;
    }

    public void destory() {
        Log.i(TAG, " OplusHeifWriter destory!");
        nativeDestory(mNativeObject);
        mNativeObject = 0;
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            if (mNativeObject != 0) {
                destory();
                mNativeObject = 0;
            }
        } finally {
            super.finalize();
        }
    }

    public static class Options {
        public byte[] fileExtender;
        public FileDescriptor gainmapFd;
        public long mcroVideoPresentationTimestampUs;
        public FileDescriptor videoFd;
        public String xmpData;

        public Options() {
        }
    }
}
