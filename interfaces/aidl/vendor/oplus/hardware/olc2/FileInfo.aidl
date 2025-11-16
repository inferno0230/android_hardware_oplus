/*
 * SPDX-FileCopyrightText: 2025 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package vendor.oplus.hardware.olc2;

@VintfStability
parcelable FileInfo {
    String path;
    int mode;
    long size;
    long mtime;
}
