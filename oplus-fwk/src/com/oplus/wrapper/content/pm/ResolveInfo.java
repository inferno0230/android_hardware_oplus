package com.oplus.wrapper.content.pm;

public class ResolveInfo {

    private final android.content.pm.ResolveInfo mResolveInfo;

    public ResolveInfo(android.content.pm.ResolveInfo resolveInfo) {
        mResolveInfo = resolveInfo;
    }

    public int getTargetUserId() {
        return mResolveInfo.targetUserId;
    }
}
