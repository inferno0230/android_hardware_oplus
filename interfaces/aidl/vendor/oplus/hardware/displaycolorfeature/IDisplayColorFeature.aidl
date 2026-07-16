/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package vendor.oplus.hardware.displaycolorfeature;

import vendor.oplus.hardware.displaycolorfeature.DisplayColorFeatureType;

@VintfStability
interface IDisplayColorFeature {
    int getDisplayColorFeatureValue(DisplayColorFeatureType type, out int[] values);
    int setDisplayColorFeatureValue(DisplayColorFeatureType type, in int[] values);
}
