package com.oplus.zoomwindow;

import android.os.Bundle;

import android.graphics.Rect;

import android.os.Parcel;
import android.os.Parcelable;

public class OplusZoomWindowInfo implements Parcelable {

    public String zoomPkg = "";
    public String cpnName = "";
    public String lockPkg = "";
    public int zoomUserId;
    public int lockUserId;
    public int rotation;
    public int lastExitMethod;
    public int inputMethodType;
    public boolean windowShown;
    public boolean inputShow;
    public Rect zoomRect = new Rect();
    public Bundle extension = new Bundle();

    public static final Parcelable.Creator<OplusZoomWindowInfo> CREATOR =
            new Parcelable.Creator<OplusZoomWindowInfo>() {

        @Override
        public OplusZoomWindowInfo createFromParcel(Parcel source) {
            return new OplusZoomWindowInfo(source);
        }

        @Override
        public OplusZoomWindowInfo[] newArray(int size) {
            return new OplusZoomWindowInfo[size];
        }
    };

    public OplusZoomWindowInfo() {}

    public OplusZoomWindowInfo(Parcel in) {}

    public OplusZoomWindowInfo(OplusZoomWindowInfo in) {}

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {}

    public void readFromParcel(Parcel in) {}
}
