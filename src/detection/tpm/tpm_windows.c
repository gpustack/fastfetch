#include "tpm.h"
#include "common/library.h"

// #include <tbs.h>

#define TBS_SUCCESS 0u
#define TBS_E_INTERNAL_ERROR 0x80284001u
#define TBS_E_INVALID_CONTEXT 0x80284004u
typedef UINT32 TBS_RESULT;

#define TPM_VERSION_UNKNOWN     0
#define TPM_VERSION_12          1
#define TPM_VERSION_20          2

#define TPM_IFTYPE_UNKNOWN        0
#define TPM_IFTYPE_1              1 // for 1.2 - use I/O-port or MMIO
#define TPM_IFTYPE_TRUSTZONE      2 // 2.0: Trustzone
#define TPM_IFTYPE_HW             3 // 2.0: HW TPM
#define TPM_IFTYPE_EMULATOR       4 // 2.0: SW-emulator
#define TPM_IFTYPE_SPB            5 // 2.0: SPB attached

typedef struct _TPM_DEVICE_INFO
{
    UINT32      structVersion;      // = 1 for now
    UINT32      tpmVersion;         // 1.2 / 2.0
    UINT32      tpmInterfaceType;   // HW, simulator, ...
    UINT32      tpmImpRevision;     // code-drop revision,
                                    // implenmentation-specific
} TPM_DEVICE_INFO, *PTPM_DEVICE_INFO;
typedef const TPM_DEVICE_INFO *PCTPM_DEVICE_INFO;

TBS_RESULT WINAPI
Tbsi_GetDeviceInfo(
    _In_ UINT32 Size,
    _Out_writes_bytes_(Size) PVOID Info);

const char* ffDetectTPM(FFTPMResult* result)
{
    FF_LIBRARY_LOAD(tbs, "dlopen TBS" FF_LIBRARY_EXTENSION " failed", "TBS" FF_LIBRARY_EXTENSION, -1)
    FF_LIBRARY_LOAD_SYMBOL_MESSAGE(tbs, Tbsi_GetDeviceInfo)

    TPM_DEVICE_INFO deviceInfo = {};
    TBS_RESULT code = ffTbsi_GetDeviceInfo(sizeof(deviceInfo), &deviceInfo);
    if (code != TBS_SUCCESS)
        return code == (TBS_RESULT) TBS_E_INVALID_CONTEXT ? "TPM device is not found" : "Tbsi_GetDeviceInfo() failed";

    switch (deviceInfo.tpmVersion)
    {
    case TPM_VERSION_12:
        ffStrbufSetStatic(&result->version, "1.2");
        break;
    case TPM_VERSION_20:
        ffStrbufSetStatic(&result->version, "2.0");
        break;
    default:
        ffStrbufSetStatic(&result->version, "unknown");
        break;
    }

    switch (deviceInfo.tpmInterfaceType)
    {
    case TPM_IFTYPE_1:
        ffStrbufSetStatic(&result->interfaceType, "I/O-port or MMIO");
        break;
    case TPM_IFTYPE_TRUSTZONE:
        ffStrbufSetStatic(&result->interfaceType, "Trustzone");
        break;
    case TPM_IFTYPE_HW:
        ffStrbufSetStatic(&result->interfaceType, "HW TPM");
        break;
    case TPM_IFTYPE_EMULATOR:
        ffStrbufSetStatic(&result->interfaceType, "SW-emulator");
        break;
    case TPM_IFTYPE_SPB:
        ffStrbufSetStatic(&result->interfaceType, "SPB attached");
        break;
    default:
        break;
    }

    return NULL;
}
