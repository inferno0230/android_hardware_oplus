/*
 * SPDX-FileCopyrightText: 2025 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package vendor.oplus.hardware.olc2;

import vendor.oplus.hardware.olc2.ExceptionInfo;
import vendor.oplus.hardware.olc2.ExceptionRecord;

@VintfStability
interface IOplusLogCoreEventCallback {
    void onDroppedExceptions(in ExceptionRecord[] records);
    void onException(in ExceptionInfo exception);
}
