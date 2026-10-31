package com.oplus.resolver;

import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.drawable.Drawable;
import com.oplus.widget.OplusItem;

/**
 * AOSP-backed implementation of the resolver custom-icon helper.
 *
 * The stock ColorOS OplusResolverCustomIconHelper resolves themed/UX icons through
 * private framework internals (IPackageManagerExt#getUxIconPackageManagerExt,
 * IOplusThemeManager, OplusResolverInfoHelper, ...) that do not exist on AOSP.
 *
 * This port keeps only the part the gallery share grid actually needs: a non-null
 * OplusItem carrying the app's icon + label, loaded purely via the standard
 * PackageManager / ResolveInfo APIs. OplusGalleryLoadIconHelper picks this class up
 * by reflection (FEATURE_PACKAGE); without a populated icon here, the gallery's
 * ResolveInfoLoader produces an "empty" item and never attaches the target click
 * listener (dead share targets).
 */
public class OplusResolverCustomIconHelper implements IOplusResolverCustomIconHelper {
    private static final String TAG = "OplusResolverCustomIconHelper";
    private final Context mContext;

    public OplusResolverCustomIconHelper(Context context, int compatibleArgs) {
        this(context);
    }

    public OplusResolverCustomIconHelper(Context context) {
        this.mContext = context;
    }

    @Override
    public OplusItem getAppInfo(Intent originIntent, ResolveInfo info, PackageManager mPm) {
        if (info == null || mPm == null) {
            return null;
        }
        OplusItem appInfo = new OplusItem();
        appInfo.setText(info.loadLabel(mPm).toString());
        if (info.activityInfo != null) {
            appInfo.setPackageName(info.activityInfo.packageName);
            ApplicationInfo applicationInfo = info.activityInfo.applicationInfo;
            if (applicationInfo != null) {
                String label = applicationInfo.loadLabel(mPm).toString();
                if (!appInfo.getText().equals(label)) {
                    appInfo.setLabel(label);
                }
            }
        }
        appInfo.setIcon(oplusLoadIconForResolveInfo(originIntent, info, mPm));
        return appInfo;
    }

    @Override
    public Drawable oplusLoadIconForResolveInfo(Intent originIntent, ResolveInfo info, PackageManager mPm) {
        if (info == null || mPm == null) {
            return null;
        }
        return info.loadIcon(mPm);
    }
}
