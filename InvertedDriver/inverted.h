///////////////////////////////////////////////////////////////////////////////
//
//    (C) Copyright 1995-2013 OSR Open Systems Resources, Inc. All Rights
//    Reserved
//
//    This software is supplied for instructional purposes only.
//
//    OSR Open Systems Resources, Inc. (OSR) expressly disclaims any warranty
//    for this software.  THIS SOFTWARE IS PROVIDED  "AS IS" WITHOUT WARRANTY
//    OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION,
//    THE IMPLIED WARRANTIES OF MECHANTABILITY OR FITNESS FOR A PARTICULAR
//    PURPOSE.  THE ENTIRE RISK ARISING FROM THE USE OF THIS SOFTWARE REMAINS
//    WITH YOU.  OSR's entire liability and your exclusive remedy shall not
//    exceed the price paid for this material.  In no event shall OSR or its
//    suppliers be liable for any damages whatsoever (including, without
//    limitation, damages for loss of business profit, business interruption,
//    loss of business information, or any other pecuniary loss) arising out
//    of the use or inability to use this software, even if OSR has been
//    advised of the possibility of such damages.  Because some states/
//    jurisdictions do not allow the exclusion or limitation of liability for
//    consequential or incidental damages, the above limitation may not apply
//    to you.
//
//    OSR Open Systems Resources, Inc.
//    105 Route 101A Suite 19
//    Amherst, NH 03031  (603) 595-6500 FAX: (603) 595-6503
//    email bugs to: bugs@osr.com
//
//
//    MODULE:
//
//        INVERTED.H -- General header for OSR Inverted KMDF example driver
//
//    ABSTRACT:
//  
//
//    AUTHOR(S):
//
//        OSR Open Systems Resources, Inc.
// 
///////////////////////////////////////////////////////////////////////////////
extern "C" {
#include <ntddk.h> 
#include <wdf.h>

DRIVER_INITIALIZE DriverEntry;

}

#include "inverted_ioctl.h"

//
// Device protection string
// This is the ACL used for the device object created by the driver.
//
DECLARE_CONST_UNICODE_STRING(
INVERTED_DEVICE_PROTECTION,
    L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGWGX;;;WD)(A;;GRGWGX;;;RC)");
extern const UNICODE_STRING  INVERTED_DEVICE_PROTECTION;

//
// Inverted device context structure
//
// KMDF will associate this structure with each "Inverted" device that
// this driver creates.
//
typedef struct _INVERTED_DEVICE_CONTEXT  {
    WDFQUEUE    NotificationQueue;
    LONG       Sequence;
} INVERTED_DEVICE_CONTEXT, * PINVERTED_DEVICE_CONTEXT;

//
// Accessor structure
//
// Given a WDFDEVICE handle, we'll use the following function to return
// a pointer to our device's context area.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(INVERTED_DEVICE_CONTEXT, InvertedGetContextFromDevice)

//
// Forward declarations
//
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD InvertedEvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL InvertedEvtIoDeviceControl;

VOID
InvertedNotify(PINVERTED_DEVICE_CONTEXT DevContext);

