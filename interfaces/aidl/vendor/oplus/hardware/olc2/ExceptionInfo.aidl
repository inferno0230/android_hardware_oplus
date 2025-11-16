/*
 * SPDX-FileCopyrightText: 2025 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package vendor.oplus.hardware.olc2;

@VintfStability
parcelable ExceptionInfo {
    long time;
    int exceptionId;
    int exceptionType;
    int level;
    long atomicLogs;
    String logParams;
}
