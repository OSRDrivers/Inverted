//
// INVERTED_IOCTL.H
//

//
// This header file contains all declarations shared between driver and user
// applications.
//

#pragma once

//
// The following value is arbitrarily chosen from the space defined by Microsoft
// as being "for non-Microsoft use"
//
#define FILE_DEVICE_INVERTED 0xCF54

//
// Device control codes - values between 2048 and 4095 arbitrarily chosen
//
#define IOCTL_OSR_INVERT_NOTIFICATION CTL_CODE(FILE_DEVICE_INVERTED, 2049, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_OSR_INVERT_SIMULATE_EVENT_OCCURRED CTL_CODE(FILE_DEVICE_INVERTED, 2050, METHOD_BUFFERED, FILE_ANY_ACCESS)

