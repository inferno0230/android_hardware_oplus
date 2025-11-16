/*
 * SPDX-FileCopyrightText: 2025 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package vendor.oplus.hardware.olc2;
@VintfStability
interface IOplusLogCore {
  int enableExceptionMonitor(boolean enabled);
  int olcRaiseException(in vendor.oplus.hardware.olc2.ExceptionInfo exceptionInfo);
  int pullDroppedExceptions();
  int registerEventCallback(vendor.oplus.hardware.olc2.IOplusLogCoreEventCallback callback);
  int unregisterEventCallback(vendor.oplus.hardware.olc2.IOplusLogCoreEventCallback callback);
  boolean doShell(String command);
  boolean doShellBlocking(String command);
  ParcelFileDescriptor getFileDescriptor(String path, int flags);
  vendor.oplus.hardware.olc2.FileInfo getFileInfo(String path);
  List<vendor.oplus.hardware.olc2.FileInfo> getFileInfoList(String path, boolean recursive);
  boolean removePath(String path);
  void registerGaiaEventListener(vendor.oplus.hardware.olc2.IGaiaEventListener listener);
  void unregisterGaiaEventListener(vendor.oplus.hardware.olc2.IGaiaEventListener listener);
  void sendGaiaEvent(in byte[] event);
}
