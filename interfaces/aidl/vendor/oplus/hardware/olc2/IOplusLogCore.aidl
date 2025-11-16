/*
 * SPDX-FileCopyrightText: 2025 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package vendor.oplus.hardware.olc2;

import vendor.oplus.hardware.olc2.ExceptionInfo;
import vendor.oplus.hardware.olc2.IOplusLogCoreEventCallback;

@VintfStability
interface IOplusLogCore {
    int enableExceptionMonitor(boolean enabled);
    int olcRaiseException(in ExceptionInfo exceptionInfo);
    int pullDroppedExceptions();

    int registerEventCallback(IOplusLogCoreEventCallback callback);
    int unregisterEventCallback(IOplusLogCoreEventCallback callback);

    boolean doShell(String command);
    boolean doShellBlocking(String command);
}
