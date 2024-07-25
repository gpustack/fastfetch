#include "gpu.h"
#include "detection/gpu/gpu_driver_specific.h"
#include "util/windows/unicode.h"
#include "util/windows/registry.h"

#include <inttypes.h>

static inline bool getDriverSpecificDetectionFn(const char* vendor, __typeof__(&ffDetectNvidiaGpuInfo)* pDetectFn, const char** pDllName)
{
    if (vendor == FF_GPU_VENDOR_NAME_NVIDIA)
    {
        *pDetectFn = ffDetectNvidiaGpuInfo;
        *pDllName = "nvml.dll";
    }
    else if (vendor == FF_GPU_VENDOR_NAME_INTEL)
    {
        *pDetectFn = ffDetectIntelGpuInfo;
        #ifdef _WIN64
            *pDllName = "ControlLib.dll";
        #else
            *pDllName = "ControlLib32.dll";
        #endif
    }
    else if (vendor == FF_GPU_VENDOR_NAME_AMD)
    {
        *pDetectFn = ffDetectAmdGpuInfo;
        #ifdef _WIN64
            *pDllName = "amd_ags_x64.dll";
        #else
            *pDllName = "amd_ags_x86.dll";
        #endif
    }
    else
    {
        *pDetectFn = NULL;
        *pDllName = NULL;
        return false;
    }

    return true;
}

const char* ffDetectGPUImpl(FF_MAYBE_UNUSED const FFGPUOptions* options, FFlist* gpus)
{

    // temporary support nvidia.
    const char* dllName = "nvml.dll";

    uint32_t gpuCount = FF_GPU_COUNT_UNSET;
    if (!ffDetectNvidiaGpuCount(&gpuCount, dllName)){
         
        for (uint32_t i = 0; i < gpuCount; ++i){
            
            FFGPUResult* gpu = (FFGPUResult*)ffListAdd(gpus);
            ffStrbufInitStatic(&gpu->vendor, FF_GPU_VENDOR_NAME_NVIDIA);
            ffStrbufInitStatic(&gpu->platformApi, "NVML");
            
            ffStrbufInit(&gpu->name);
            ffStrbufInit(&gpu->uuid);
            ffStrbufInit(&gpu->driver);
            
            gpu->index = FF_GPU_INDEX_UNSET;
            gpu->temperature = FF_GPU_TEMP_UNSET;
            gpu->coreCount = FF_GPU_CORE_COUNT_UNSET;
            gpu->type = FF_GPU_TYPE_UNKNOWN;
            gpu->dedicated.total = gpu->dedicated.used = gpu->shared.total = gpu->shared.used = FF_GPU_VMEM_SIZE_UNSET;
            gpu->deviceId = 0;
            gpu->frequency = FF_GPU_FREQUENCY_UNSET;
            gpu->coreUtilizationRate = FF_GPU_CORE_UTILIZATION_RATE_UNSET;

            __typeof__(&ffDetectNvidiaGpuInfo) detectFn;
            
            if (getDriverSpecificDetectionFn(gpu->vendor.chars, &detectFn, &dllName) && (options->temp || options->driverSpecific))
            {

                detectFn(
                    &(FFGpuDriverCondition){
                        .type = FF_GPU_DRIVER_CONDITION_TYPE_INDEX,
                        .index = (int32_t)i,
                    },
                    (FFGpuDriverResult){
                        .index = &gpu->index,
                        .temp = options->temp ? &gpu->temperature : NULL,
                        .memory = options->driverSpecific ? &gpu->dedicated : NULL,
                        .coreCount = options->driverSpecific ? (uint32_t *)&gpu->coreCount : NULL,
                        .type = &gpu->type,
                        .frequency = options->driverSpecific ? &gpu->frequency : NULL,
                        .coreUtilizationRate = &gpu->coreUtilizationRate,
                        .uuid = &gpu->uuid,
                        .name = &gpu->name,
                    },
                    dllName);
            }
        }
    }

    return NULL;
}
