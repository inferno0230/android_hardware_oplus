/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package org.lineageos.settings.otg;

import android.content.Context;

import com.android.settingslib.drawer.CategoryKey;
import com.android.settingslib.drawer.DynamicSummary;
import com.android.settingslib.drawer.SwitchController;
import com.android.settingslib.drawer.SwitchesProvider;

import org.lineageos.settings.R;

import java.util.Collections;
import java.util.List;

public class ForceOtgSwitchesProvider extends SwitchesProvider {
    private static final String KEY_FORCE_OTG = "force_otg";

    @Override
    protected List<SwitchController> createSwitchControllers() {
        return Collections.singletonList(new ForceOtgSwitchController(getContext()));
    }

    private static final class ForceOtgSwitchController extends SwitchController
            implements DynamicSummary {
        private final Context mContext;

        ForceOtgSwitchController(Context context) {
            mContext = context;
        }

        @Override
        public String getSwitchKey() {
            return KEY_FORCE_OTG;
        }

        @Override
        protected MetaData getMetaData() {
            final MetaData metaData = new MetaData(CategoryKey.CATEGORY_SYSTEM_DEVELOPMENT);
            metaData.setOrder(-100);
            metaData.setTitle(R.string.force_otg_tile_title);
            return metaData;
        }

        @Override
        protected boolean isChecked() {
            return ForceOtgUtils.isSupported() && ForceOtgUtils.isForceOtgEnabled(mContext);
        }

        @Override
        protected boolean onCheckedChanged(boolean checked) {
            if (!ForceOtgUtils.isSupported()) {
                return false;
            }
            return ForceOtgUtils.setForceOtgEnabled(mContext, checked);
        }

        @Override
        protected String getErrorMessage(boolean attemptedChecked) {
            if (!ForceOtgUtils.isSupported()) {
                return mContext.getString(R.string.force_otg_not_supported);
            }
            return null;
        }

        @Override
        public String getDynamicSummary() {
            if (!ForceOtgUtils.isSupported()) {
                return mContext.getString(R.string.force_otg_not_supported);
            }
            return mContext.getString(R.string.force_otg_summary);
        }
    }
}
