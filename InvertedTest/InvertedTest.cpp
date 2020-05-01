//
// InvertedTest.c
//
// Win32 console mode program to fire up requests to the
// Inverted Example driver.
//
#define _CRT_SECURE_NO_WARNINGS  

#include <cstdio>
#include <cstdlib>
#include <Windows.h>
#include <inverted_ioctl.h>

typedef struct _OVL_WRAPPER {
    OVERLAPPED  Overlapped;
    LONG        ReturnedSequence;
} OVL_WRAPPER, *POVL_WRAPPER;

DWORD SendTheIoctl(POVL_WRAPPER Wrap);
DWORD WINAPI CompletionPortThread(LPVOID PortHandle);

HANDLE driverHandle;

int main()
{
    HANDLE  completionPortHandle;
    DWORD   code;
    DWORD   Function = 0;
    DWORD   dwThreadId = 0;
    HANDLE  hThread;
    POVL_WRAPPER wrap;

    //
    // Open the Inverted device by name
    //
    driverHandle = CreateFile(LR"(\\.\Inverted)",       // Name of the device to open
                              GENERIC_READ|GENERIC_WRITE,  // Access rights requested
                              0,                           // Share access - NONE
                              nullptr,                     // Security attributes - not used!
                              OPEN_EXISTING,               // Device must exist to open it.
                              FILE_FLAG_OVERLAPPED,        // Open for overlapped I/O
                              nullptr);                          // extended attributes - not used!

      //
      // If this call fails, check to figure out what the error is and report it.
      //
    if (driverHandle == INVALID_HANDLE_VALUE) {

        code = GetLastError();

        printf("CreateFile failed with error 0x%lx\n",
               code);

        return(code);
    }

    completionPortHandle = CreateIoCompletionPort(driverHandle,
                                                  nullptr,
                                                  0,
                                                  0);

    if (completionPortHandle == nullptr) {

        code = GetLastError();

        printf("CreateIoCompletionPort failed with error 0x%lx\n", code);

        return(code);

    }

    hThread = CreateThread(nullptr,               // Default thread security descriptor
                           0,                     // Default stack size
                           CompletionPortThread,  // Start routine
                           completionPortHandle,  // Start routine parameter
                           0,                     // Run immediately
                           &dwThreadId);          // Thread ID

    if (hThread == nullptr) {
        code = GetLastError();

        printf("CreateThread failed with error 0x%lx\n", code);

        return(code);
    }

    //
    // Infinitely print out the list of choices, ask for input, process
    // the request
    //
    printf("\nINVERTED CALL TEST -- Functions:\n");

    while (TRUE) {

        printf("\n\t1. Asynchronously send IOCTL_OSR_INVERT_NOTIFICATION\n");
        printf("\t2. Cause an event notification\n");
        printf("\n\t0. Exit\n");
        printf("\n\tSelection: ");
        scanf("%lx", &Function);

        switch (Function) {


            case 1:
            {
                wrap = static_cast<POVL_WRAPPER>(malloc(sizeof(OVL_WRAPPER)));
                memset(wrap, 0, sizeof(OVL_WRAPPER));

                //
                // Test the IOCTL interface
                //
                DeviceIoControl(driverHandle,
                                static_cast<DWORD>(IOCTL_OSR_INVERT_NOTIFICATION),
                                nullptr,                      // Ptr to InBuffer
                                0,                            // Length of InBuffer
                                &wrap->ReturnedSequence,      // Ptr to OutBuffer
                                sizeof(LONG),                 // Length of OutBuffer
                                nullptr,                      // BytesReturned
                                &wrap->Overlapped);           // Ptr to Overlapped structure

                code = GetLastError();

                if (code != ERROR_IO_PENDING) {

                    printf("DeviceIoControl failed with error 0x%lx\n", code);

                    return(code);

                }

                break;
            }

            case 2:
            {
                wrap = static_cast<POVL_WRAPPER>(malloc(sizeof(OVL_WRAPPER)));
                memset(wrap, 0, sizeof(OVL_WRAPPER));

                if (!DeviceIoControl(driverHandle,
                                     static_cast<DWORD>(IOCTL_OSR_INVERT_SIMULATE_EVENT_OCCURRED),
                                     nullptr,                         // Ptr to InBuffer
                                     0,                            // Length of InBuffer
                                     nullptr,                         // Ptr to OutBuffer
                                     0,                            // Length of OutBuffer
                                     nullptr,                         // BytesReturned
                                     &wrap->Overlapped))          // Ptr to Overlapped structure
                {

                    code = GetLastError();

                    if (code != ERROR_IO_PENDING) {

                        printf("DeviceIoControl failed with error 0x%lx\n", code);

                        return(code);

                    }
                }
                break;

            }

            case 0:
            {
                //
                // zero is get out!
                //
                return(0);
            }

            default:
            {
                //
                // Just re-prompt for anything else
                //
                break;
            }

        }
    }
}

DWORD
WINAPI CompletionPortThread(LPVOID PortHandle)
{
    DWORD byteCount = 0;
    ULONG_PTR compKey = 0;
    OVERLAPPED* overlapped = nullptr;
    POVL_WRAPPER wrap;
    DWORD code;

    while (TRUE) {

        // Wait for a completion notification.
        overlapped = nullptr;

        BOOL worked = GetQueuedCompletionStatus(PortHandle,                // Completion port handle
                                                &byteCount,                // Bytes transferred
                                                &compKey,                  // Completion key... don't care
                                                &overlapped,               // OVERLAPPED structure
                                                INFINITE);                 // Notification time-out interval

        //
        // If it's our notification ioctl that's just been completed...
        // don't do anything special.
        // 
        if (byteCount == 0) {
            continue;
        }

        if (overlapped == nullptr) {

            // An unrecoverable error occurred in the completion port.
            // Wait for the next notification.
            continue;
        }

        //
        // Because the wrapper structure STARTS with the OVERLAPPED structure,
        // the pointers are the same.  It would be nicer to use
        // CONTAINING_RECORD here... however you do that in user-mode.
        // 
        wrap = reinterpret_cast<POVL_WRAPPER>(overlapped);

        code = GetLastError();

        printf(">>> Notification received.  Sequence = %ld\n",
               wrap->ReturnedSequence);
    }
}
