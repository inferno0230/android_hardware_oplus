/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package org.lineageos.settings.ltpo;

import android.content.Context;
import android.os.Bundle;

import com.android.settingslib.drawer.CategoryKey;
import com.android.settingslib.drawer.DynamicSummary;
import com.android.settingslib.drawer.SwitchController;
import com.android.settingslib.drawer.SwitchesProvider;
import com.android.settingslib.drawer.TileUtils;

import org.lineageos.settings.R;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class LtpoSwitchesProvider extends SwitchesProvider {
    private static final String KEY_LTPO = "ltpo";

    @Override
    public Bundle call(String method, String uriString, Bundle extras) {
        final boolean isDiscoveryCall = extras == null
                || !extras.containsKey(TileUtils.META_DATA_PREFERENCE_KEYHINT);
        if (isDiscoveryCall && !LtpoHelper.isSupported()) {
            if (METHOD_GET_ENTRY_DATA.equals(method)) {
                final Bundle bundle = new Bundle();
                bundle.putParcelableList(EXTRA_ENTRY_DATA, new ArrayList<Bundle>());
                return bundle;
            } else if (METHOD_GET_SWITCH_DATA.equals(method)) {
                final Bundle bundle = new Bundle();
                bundle.putParcelableList(EXTRA_SWITCH_DATA, new ArrayList<Bundle>());
                return bundle;
            }
        }

        return super.call(method, uriString, extras);
    }

    @Override
    protected List<SwitchController> createSwitchControllers() {
        return Collections.singletonList(new LtpoSwitchController(getContext()));
    }

    private static final class LtpoSwitchController extends SwitchController
            implements DynamicSummary {
        private final Context mContext;

        LtpoSwitchController(Context context) {
            mContext = context;
        }

        @Override
        public String getSwitchKey() {
            return KEY_LTPO;
        }

        @Override
        protected MetaData getMetaData() {
            final MetaData metaData = new MetaData(CategoryKey.CATEGORY_DISPLAY);
            metaData.setTitle(R.string.ltpo_title);
            return metaData;
        }

        @Override
        protected boolean isChecked() {
            return LtpoHelper.isSupported() && LtpoHelper.isLtpoEnabled(mContext);
        }

        @Override
        protected boolean onCheckedChanged(boolean checked) {
            if (!LtpoHelper.isSupported()) {
                return false;
            }

            return LtpoHelper.setLtpoEnabled(mContext, checked);
        }

        @Override
        protected String getErrorMessage(boolean attemptedChecked) {
            if (!LtpoHelper.isSupported()) {
                return mContext.getString(R.string.ltpo_not_supported);
            }

            return null;
        }

        @Override
        public String getDynamicSummary() {
            if (!LtpoHelper.isSupported()) {
                return mContext.getString(R.string.ltpo_not_supported);
            }

            return mContext.getString(R.string.ltpo_summary);
        }
    }
}
