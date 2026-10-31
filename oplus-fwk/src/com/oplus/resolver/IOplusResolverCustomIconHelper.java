package com.oplus.resolver;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.drawable.Drawable;
import com.oplus.widget.OplusItem;

/* loaded from: classes.dex */
public interface IOplusResolverCustomIconHelper {
    public static final IOplusResolverCustomIconHelper DEFAULT = new IOplusResolverCustomIconHelper() { // from class: com.oplus.resolver.IOplusResolverCustomIconHelper.1
    };

    default OplusItem getAppInfo(Intent originIntent, ResolveInfo info, PackageManager mPm) {
        return new OplusItem();
    }

    default Drawable oplusLoadIconForResolveInfo(Intent originIntent, ResolveInfo info, PackageManager mPm) {
        return null;
    }
}
