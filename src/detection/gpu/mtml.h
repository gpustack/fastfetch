#pragma once

// DISCLAIMER:
// THIS FILE IS CREATED FROM SCRATCH, BY READING THE OFFICIAL MTML API
// DOCUMENTATION REFERENCED BELOW, IN ORDER TO MAKE FASTFETCH MIT COMPLIANT.

#define MTML_API __attribute__((visibility("default")))
#define MTML_DEVICE_PCI_SBDF_BUFFER_SIZE 32
#define MTML_DEVICE_NAME_BUFFER_SIZE 32
#define MTML_DEVICE_UUID_BUFFER_SIZE 48

typedef enum
{
    MTML_SUCCESS = 0,
} MtmlReturn;

typedef enum
{
    MTML_BRAND_MTT = 0, //!< MTT series.
    MTML_BRAND_UNKNOWN, //!< An unknown brand.

    // Keep this on the last line.
    MTML_BRAND_COUNT //!< The number of brands.
} MtmlBrandType;

typedef struct MtmlLibrary MtmlLibrary;
typedef struct MtmlSystem MtmlSystem;
typedef struct MtmlDevice MtmlDevice;
typedef struct MtmlGpu MtmlGpu;
typedef struct MtmlMemory MtmlMemory;

typedef struct
{
    char sbdf[MTML_DEVICE_PCI_SBDF_BUFFER_SIZE]; //!< The tuple segment:bus:device.function PCI identifier (&amp; NULL terminator).
    unsigned int segment;                        //!< The PCI segment group(domain) on which the device's bus resides, 0 to 0xffffffff.
    unsigned int bus;                            //!< The bus on which the device resides, 0 to 0xff.
    unsigned int device;                         //!< The device ID on the bus, 0 to 31.
    unsigned int pciDeviceId;                    //!< The combined 16-bit device ID and 16-bit vendor ID.
    unsigned int pciSubsystemId;                 //!< The 32-bit sub system device ID.
    unsigned int busWidth;                       //!< @deprecated This value set to zero.
    float pciMaxSpeed;                           //!< The maximum link speed (transfer rate per lane) of the device. The unit is GT/s.
    float pciCurSpeed;                           //!< The current link speed (transfer rate per lane) of the device. The unit is GT/s.
    unsigned int pciMaxWidth;                    //!< The maximum link width of the device.
    unsigned int pciCurWidth;                    //!< The current link width of the device.
    unsigned int pciMaxGen;                      //!< The maximum supported generation of the device.
    unsigned int pciCurGen;                      //!< The current generation of the device.
    int rsvd[6];                                 //!< Reserved for future extension.
} MtmlPciInfo;

MtmlReturn MTML_API mtmlDeviceGetBrand(const MtmlDevice *dev, MtmlBrandType *type);
MtmlReturn MTML_API mtmlDeviceGetIndex(const MtmlDevice *dev, unsigned int *index);
MtmlReturn MTML_API mtmlDeviceGetName(const MtmlDevice *dev, char *name, unsigned int length);
MtmlReturn MTML_API mtmlDeviceGetPciInfo(const MtmlDevice *dev, MtmlPciInfo *pci);
MtmlReturn MTML_API mtmlDeviceGetUUID(const MtmlDevice *dev, char *uuid, unsigned int length);
MtmlReturn MTML_API mtmlDeviceInitGpu(const MtmlDevice *dev, MtmlGpu **gpu);
MtmlReturn MTML_API mtmlDeviceInitMemory(const MtmlDevice *dev, MtmlMemory **mem);

MtmlReturn MTML_API mtmlGpuGetMaxClock(const MtmlGpu *gpu, unsigned int *clockMhz);
MtmlReturn MTML_API mtmlGpuGetTemperature(const MtmlGpu *gpu, unsigned int *temp);
MtmlReturn MTML_API mtmlGpuGetUtilization(const MtmlGpu *gpu, unsigned int *utilization);

MtmlReturn MTML_API mtmlLibraryCountDevice(const MtmlLibrary *lib, unsigned int *count);
MtmlReturn MTML_API mtmlLibraryInit(MtmlLibrary **lib);
MtmlReturn MTML_API mtmlLibraryInitDeviceByIndex(const MtmlLibrary *lib, unsigned int index, MtmlDevice **dev);
MtmlReturn MTML_API mtmlLibraryInitDeviceByPciSbdf(const MtmlLibrary *lib, const char *pciSbdf, MtmlDevice **dev);
MtmlReturn MTML_API mtmlLibraryInitSystem(const MtmlLibrary *lib, MtmlSystem **sys);
MtmlReturn MTML_API mtmlLibraryShutDown(MtmlLibrary *lib);

MtmlReturn MTML_API mtmlMemoryGetTotal(const MtmlMemory *mem, unsigned long long *total);
MtmlReturn MTML_API mtmlMemoryGetUsed(const MtmlMemory *mem, unsigned long long *used);
MtmlReturn MTML_API mtmlMemoryGetUtilization(const MtmlMemory *mem, unsigned int *utilization);
