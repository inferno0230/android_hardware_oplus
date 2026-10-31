package com.oplus.wrapper.os;

public class UserManager {

    private final android.os.UserManager mUserManager;

    public UserManager(android.os.UserManager userManager) {
        mUserManager = userManager;
    }

    public boolean isGuestUser() {
        return mUserManager.isGuestUser();
    }
}
