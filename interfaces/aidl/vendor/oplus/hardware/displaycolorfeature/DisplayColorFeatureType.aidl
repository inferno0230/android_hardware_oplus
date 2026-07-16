/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package vendor.oplus.hardware.displaycolorfeature;

@Backing(type="int")
@VintfStability
enum DisplayColorFeatureType {
    PostBlendPCC = 1,
    PostBlendPa = 2,
    PostBlendDither = 3,
    PostBlendPaDither = 4,
    PostBlendDE = 5,
    PostBlendGC = 6,
    PostBlendGamut = 7,
    PostBlendIGC = 8,
    kStcDebugEnable = 19,
    ClstcDetailEnhancer = 20,
    ClstcIgcFeature = 21,
    ClstcCscFeature = 22,
    ClstcGcFeature = 23,
    ClstcdLutFeature = 24,
    kClstcDebugEnable = 30,
}
