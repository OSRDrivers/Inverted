///////////////////////////////////////////////////////////////////////////////
//
//    (C) Copyright 1995 - 2013 OSR Open Systems Resources, Inc.
//    All Rights Reserved
//
//    This software is supplied for instructional purposes only.
//
//    OSR Open Systems Resources, Inc. (OSR) expressly disclaims any warranty
//    for this software.  THIS SOFTWARE IS PROVIDED  "AS IS" WITHOUT WARRANTY
//    OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION,
//    THE IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR
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
//    MODULE:
//
//        InvertedExample.cpp
//
//    ABSTRACT:
//
//      This file contains the skeleton code for a KMDF driver
//      that demonstrates use of the Inverted Call Model.
//
//    AUTHOR(S):
//
//        OSR Open Systems Resources, Inc.
// 
//    REVISION:   
//
//         Original version
//         
///////////////////////////////////////////////////////////////////////////////
#include "inverted.h"

///////////////////////////////////////////////////////////////////////////////
//
//  DriverEntry
//
//    This routine is called by Windows when the driver is first loaded.  It
//    is the responsibility of this routine to create the WDFDRIVER
//
//  INPUTS:
//
//      DriverObject - Address of the DRIVER_OBJECT created by Windows for this
//                     driver.
//
//      RegistryPath - UNICODE_STRING which represents this driver's key in the
//                     Registry.  
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      STATUS_SUCCESS, otherwise an error indicating why the driver could not
//                      load.
//
//  IRQL:
//
//      This routine is called at IRQL == PASSIVE_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

#if DBG
    DbgPrint("\nOSR InvertedExample Driver -- Compiled %s %s\n",__DATE__, __TIME__);
#endif

    //
    // Provide pointer to our EvtDeviceAdd event processing callback
    // function
    //
    WDF_DRIVER_CONFIG_INIT(&config, InvertedEvtDeviceAdd);


    //
    // Create our WDFDriver instance
    //
    status = WdfDriverCreate(DriverObject,
                        RegistryPath,
                        WDF_NO_OBJECT_ATTRIBUTES, 
                        &config,     
                        WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("WdfDriverCreate failed 0x%0x\n", status);
#endif
    }

    return(status);
}

///////////////////////////////////////////////////////////////////////////////
//
//  InvertedEvtDeviceAdd
//
//    This routine is called by the framework when a device of
//    the type we support is found in the system.
//
//  INPUTS:
//
//      DriverObject - Our WDFDRIVER object
//
//      DeviceInit   - The device initialization structure we'll
//                     be using to create our WDFDEVICE
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      STATUS_SUCCESS, otherwise an error indicating why the driver could not
//                      load.
//
//  IRQL:
//
//      This routine is called at IRQL == PASSIVE_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
NTSTATUS
InvertedEvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES objAttributes;
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG queueConfig;
    PINVERTED_DEVICE_CONTEXT devContext;
    
    //
    // Our user-accessible device name
    //
    // Note that we only support a single instance of this device being
    // created. This is an example driver, after all.
    //
    DECLARE_CONST_UNICODE_STRING(userDeviceName, L"\\Global??\\Inverted");
    
    UNREFERENCED_PARAMETER(Driver);

    //
    // Life is nice and simple in this driver... 
    //
    // We don't need ANY PnP/Power Event Processing callbacks. There's no
    // hardware, thus we don't need EvtPrepareHardware or EvtReleaseHardware 
    // There's no power state to handle so we don't need EvtD0Entry or EvtD0Exit.
    //
    
    //
    // Prepare for WDFDEVICE creation
    //
    // Initialize standard WDF Object Attributes structure
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&objAttributes);

    //
    // Specify our device context
    //
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&objAttributes,
                                           INVERTED_DEVICE_CONTEXT);

    //
    // Let's create our device object
    //
    status = WdfDeviceCreate(&DeviceInit,
                            &objAttributes, 
                            &device);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("WdfDeviceCreate failed 0x%0x\n", status);
#endif
        return status;
    }

    //
    // Now that our device has been created, get our per-device-instance
    // storage area
    // 
    devContext = InvertedGetContextFromDevice(device);

    //
    // Initialize the sequence number we'll use for something to 
    // return during notification
    // 
    devContext->Sequence = 1;

    //
    // Create a symbolic link for our device so that user-mode can open
    // the device by name.
    //
    status = WdfDeviceCreateSymbolicLink(device, &userDeviceName);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("WdfDeviceCreateSymbolicLink failed 0x%0x\n", status);
#endif
        return(status);
    }

    //
    // Configure our queue of incoming requests
    //
    // We only use our default Queue to receive requests from the Framework,
    // and we set it for parallel processing.  This means that the driver can
    // have multiple requests outstanding from this queue simultaneously.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig,
                                           WdfIoQueueDispatchParallel);

    //
    // Declare our I/O Event Processing callbacks
    //
    // This driver only handles IOCTLs.
    //
    // WDF will automagically handle Create and Close requests for us and will
    // will complete any OTHER request types with STATUS_INVALID_DEVICE_REQUEST.    
    //
    queueConfig.EvtIoDeviceControl = InvertedEvtIoDeviceControl;

    //
    // Because this is a queue for a software-only device, indicate
    // that the queue doesn't need to be power managed
    //
    queueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(device,
                              &queueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("WdfIoQueueCreate for default queue failed 0x%0x\n", status);
#endif
        return(status);
    }

    //
    // And we're going to use a Queue with manual dispatching to hold the
    // Requests that we'll use for notification purposes
    // 
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
                             WdfIoQueueDispatchManual);

    queueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(device,
                              &queueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &devContext->NotificationQueue);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("WdfIoQueueCreate for manual queue failed 0x%0x\n", status);
#endif
        return(status);
    }

    return(status);
}

///////////////////////////////////////////////////////////////////////////////
//
//  InvertedEvtIoDeviceControl
//
//    This routine is called by the framework when there is a
//    device control request for us to process
//
//  INPUTS:
//
//      Queue    - Our default queue
//
//      Request  - A device control request
//
//      OutputBufferLength - The length of the output buffer
//
//      InputBufferLength  - The length of the input buffer
//
//      IoControlCode      - The operation being performed
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//  IRQL:
//
//      This routine is called at IRQL <= DISPATCH_LEVEL
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
VOID
InvertedEvtIoDeviceControl(WDFQUEUE Queue,
            WDFREQUEST Request,
            size_t OutputBufferLength,
            size_t InputBufferLength,
            ULONG IoControlCode)
{
    PINVERTED_DEVICE_CONTEXT devContext;
    NTSTATUS status;
    ULONG_PTR info;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    devContext = InvertedGetContextFromDevice(
                                        WdfIoQueueGetDevice(Queue) );

    //
    // Set the default completion status and information field
    // 
    status = STATUS_INVALID_PARAMETER;
    info = 0;

#if DBG
    DbgPrint("InvertedEvtIoDeviceControl\n");
#endif

    switch(IoControlCode) {

        //
        // This IOCTL are sent by the user application, and will be completed
        // by the driver when an event occurs.
        // 
        case IOCTL_OSR_INVERT_NOTIFICATION: {

            //
            // We return an 32-bit value with each completion notification.
            // Be sure the user's data buffer is at least long enough for that.
            // 
            if(OutputBufferLength < sizeof(LONG)) {
                
                //
                // Not enough space? Complete the request with
                // STATUS_INVALID_PARAMETER (as set previously).
                // 
                break;
            }


            status = WdfRequestForwardToIoQueue(Request,
                                        devContext->NotificationQueue);

            //
            // If we can't forward the Request to our holding queue,
            // we have to complete it.  We'll use whatever status we get
            // back from WdfRequestForwardToIoQueue.
            // 
            if(!NT_SUCCESS(status)) {
                break;
            }

            //
            // *** RETURN HERE WITH REQUEST PENDING ***
            //     We do not break, we do not fall through.
            //
            return;
        }

        //
        // This IOCTL is sent by the application to simulate the occurrence of
        // an event.  Typically, the event would results from something
        // happening in the driver or its device.
        // 
        case IOCTL_OSR_INVERT_SIMULATE_EVENT_OCCURRED: {

            InvertedNotify(devContext);

            //
            // Regardless of the success of the notification operation, we
            // complete the event simulation IOCTL with success
            // 
            status = STATUS_SUCCESS;

            break;
        }

        default: {
#if DBG
            DbgPrint("InvertedEvtIoDeviceControl: Invalid IOCTL received\n");
#endif
            break;
        }
    
    }

    //
    // Complete the received Request
    // 
    WdfRequestCompleteWithInformation(Request,
                                    status,
                                    info);
}

///////////////////////////////////////////////////////////////////////////////
//
//  InvertedNotify
//
//    This routine is called when the driver needs to notify the
//    user-mode application that an event has occurred. In so doing, we also
//    return a value to the OutBuffer. Here, the value is just a sequence
//    number (we have to return SOMETHING) but in actual usage this might be
//    any sort of information you need to return about the event, such as a
//    code that indicates which specific event occurred.
//
//  INPUTS:
//      DevContext      A pointer to our device context
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//  IRQL:
//
//      This routine is called at IRQL <= DISPATCH_LEVEL
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
VOID
InvertedNotify(PINVERTED_DEVICE_CONTEXT DevContext)
{
    NTSTATUS status;
    ULONG_PTR info;
    WDFREQUEST notifyRequest;
    PULONG  bufferPointer;
    LONG valueToReturn;

    status = WdfIoQueueRetrieveNextRequest(DevContext->NotificationQueue,
                                           &notifyRequest);

    //
    // Be sure we got a Request
    // 
    if(!NT_SUCCESS(status)) {

        //
        // Nope!  We were NOT able to successfully remove a Request from the
        // notification queue.  Well, perhaps there aren't any right now.
        // Whatever... not much we can do in this example about this.
        // 
#if DBG
        DbgPrint("InvertedNotify: Failed to retrieve request. Status = 0x%0x\n",
                 status);
#endif
        return;
    }

    //
    // We've successfully removed a Request from the queue of pending 
    // notification IOCTLs.
    // 

    //
    // Get a a pointer to the output buffer that was passed-in with the user
    // notification IOCTL.  We'll use this to return additional info about
    // the event. The minimum size Output Buffer that we need is sizeof(LONG).
    // We don't need this call to return us what the actual size is.
    // 
    status = WdfRequestRetrieveOutputBuffer(notifyRequest,
                                            sizeof(LONG),
                                            (PVOID*)&bufferPointer,
                                            nullptr); 
    //
    // Valid OutBuffer?
    // 
    if(!NT_SUCCESS(status)) {

        //
        // The OutBuffer associated with the pending notification Request that
        // we just dequeued is somehow not valid. This doesn't really seem
        // possible, but... you know... they return you a status, you have to
        // check it and handle it.
        // 
#if DBG
        DbgPrint("InvertedNotify: WdfRequestRetrieveOutputBuffer failed.  Status = 0x%0x\n", 
                                                                    status);
#endif

        //
        // Complete the IOCTL_OSR_INVERT_NOTIFICATION with success, but
        // indicate that we're not returning any additional information.
        // 
        status = STATUS_SUCCESS;
        info = 0;

    } else {

        //
        // We successfully retrieved a Request from the notification Queue
        // AND we retrieved an output buffer into which to return some
        // additional information.
        // 

        //
        // As part of this example, we return a monotonically increasing
        // sequence number with each notification.  In a real driver, this
        // could be information (of any size) describing or identifying the
        // event.
        // 
        // Of course, you don't HAVE to return any information with the
        // notification.  In that case, you can skip the call to 
        // WdfRequestRetrieveOutputBuffer and such.
        // 
        valueToReturn = InterlockedExchangeAdd(&DevContext->Sequence,
                                               1);

        *bufferPointer = valueToReturn;

        //
        // Complete the IOCTL_OSR_INVERT_NOTIFICATION with success, indicating
        // we're returning a longword of data in the user's OutBuffer
        // 
        status = STATUS_SUCCESS;
        info = sizeof(LONG);
    }

    //
    // And now... NOTIFY the user about the event. We do this just
    // by completing the dequeued Request.
    // 
    WdfRequestCompleteWithInformation(notifyRequest, status, info);
}
