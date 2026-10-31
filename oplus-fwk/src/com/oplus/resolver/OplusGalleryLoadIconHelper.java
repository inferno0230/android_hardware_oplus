package com.oplus.resolver;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.drawable.Drawable;
import android.os.UserHandle;
import android.util.Log;
import com.oplus.widget.OplusItem;

/* loaded from: classes.dex */
public class OplusGalleryLoadIconHelper {
    private static final String FEATURE_PACKAGE = "com.oplus.resolver.OplusResolverCustomIconHelper";
    private static final String TAG = "OplusGalleryLoadIconHelper";
    private static volatile OplusGalleryLoadIconHelper sLoadIconHelper;
    private Context mContext;
    private IOplusResolverCustomIconHelper mCustomIconHelper;
    private UserHandle mUserHandle;

    public static OplusGalleryLoadIconHelper getInstance(Context context) {
        if (sLoadIconHelper == null) {
            synchronized (OplusGalleryLoadIconHelper.class) {
                if (sLoadIconHelper == null) {
                    sLoadIconHelper = new OplusGalleryLoadIconHelper(context.getApplicationContext());
                }
            }
        }
        return sLoadIconHelper;
    }

    private OplusGalleryLoadIconHelper(Context context) {
        this.mCustomIconHelper = IOplusResolverCustomIconHelper.DEFAULT;
        this.mContext = context;
        try {
            this.mCustomIconHelper = (IOplusResolverCustomIconHelper) Class.forName(FEATURE_PACKAGE).getDeclaredConstructor(Context.class).newInstance(context);
        } catch (Exception e) {
            Log.e(TAG, "cannot constructor error:" + e.getMessage());
        }
        this.mUserHandle = UserHandle.of(context.getUserId());
    }

    public Drawable loadUxIcon(Intent originIntent, ResolveInfo info) {
        PackageManager packageManager = this.mContext.getPackageManager();
        Drawable drawable = this.mCustomIconHelper.oplusLoadIconForResolveInfo(originIntent, info, packageManager);
        if (drawable != null) {
            packageManager.getUserBadgedIcon(drawable, this.mUserHandle);
        }
        return drawable;
    }

    public OplusItem getOplusItem(Intent originIntent, ResolveInfo info, PackageManager mPm) {
        OplusItem item = this.mCustomIconHelper.getAppInfo(originIntent, info, mPm);
        if (item.getIcon() != null) {
            item.setIcon(mPm.getUserBadgedIcon(item.getIcon(), this.mUserHandle));
        }
        return item;
    }

    public void updateUserHandle(UserHandle userHandle) {
        if (userHandle != null) {
            this.mUserHandle = userHandle;
        }
    }
}
