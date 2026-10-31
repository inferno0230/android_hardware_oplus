package com.oplus.wrapper.provider;

import android.content.ContentResolver;

public class Settings {

    public static class Secure {

        public static int getIntForUser(ContentResolver cr, String name, int userHandle)
                throws android.provider.Settings.SettingNotFoundException {
            return android.provider.Settings.Secure.getIntForUser(cr, name, userHandle);
        }
    }
}
