#include "gpu.h"

#import <Metal/MTLDevice.h>

#ifndef MAC_OS_VERSION_13_0
    #define MTLGPUFamilyMetal3 ((MTLGPUFamily) 5001)
#endif
#ifndef MAC_OS_X_VERSION_10_15
    #define MTLFeatureSet_macOS_GPUFamily1_v4 ((MTLFeatureSet) 10004)
    #define MTLFeatureSet_macOS_GPUFamily2_v1 ((MTLFeatureSet) 10005)
#endif

const char* ffGpuDetectMetal(FFlist* gpus)
{
    if (@available(macOS 10.13, *))
    {
        for (id<MTLDevice> device in MTLCopyAllDevices())
        {
            FFGPUResult* gpu = NULL;
            FF_LIST_FOR_EACH(FFGPUResult, x, *gpus)
            {
                if (x->deviceId == device.registryID)
                {
                    gpu = x;
                    break;
                }
            }
            if (!gpu) continue;

            #ifndef MAC_OS_X_VERSION_10_15
            if ([device supportsFeatureSet:MTLFeatureSet_macOS_GPUFamily2_v1])
                ffStrbufSetStatic(&gpu->platformApi, "Metal Feature Set 2");
            else if ([device supportsFeatureSet:MTLFeatureSet_macOS_GPUFamily1_v1])
                ffStrbufSetStatic(&gpu->platformApi, "Metal Feature Set 1");
            #else // MAC_OS_X_VERSION_10_15
            if ([device supportsFamily:MTLGPUFamilyMetal3])
                ffStrbufSetStatic(&gpu->platformApi, "Metal 3");
            else if ([device supportsFamily:MTLGPUFamilyCommon3])
                ffStrbufSetStatic(&gpu->platformApi, "Metal Common 3");
            else if ([device supportsFamily:MTLGPUFamilyCommon2])
                ffStrbufSetStatic(&gpu->platformApi, "Metal Common 2");
            else if ([device supportsFamily:MTLGPUFamilyCommon1])
                ffStrbufSetStatic(&gpu->platformApi, "Metal Common 1");

            gpu->type = device.hasUnifiedMemory ? FF_GPU_TYPE_INTEGRATED : FF_GPU_TYPE_DISCRETE;
            gpu->index = (uint8_t)device.locationNumber;
            #endif

            int registryID_string_length = snprintf(NULL, 0, "%llu", device.registryID);
            if (registryID_string_length > 0) {
                size_t registryID_buffer_size = (size_t)registryID_string_length + 1;
                char *registryID_buffer = (char *)malloc(registryID_buffer_size);
                snprintf(registryID_buffer, registryID_buffer_size, "%llu", device.registryID);
                ffStrbufAppendS(&gpu->uuid, registryID_buffer);
                free(registryID_buffer);
            }

            if (gpu->type == FF_GPU_TYPE_INTEGRATED)
            {
                gpu->shared.total = device.recommendedMaxWorkingSetSize;
                gpu->shared.used = gpu->dedicated.used;
                
                gpu->dedicated.total = FF_GPU_VMEM_SIZE_UNSET;
                gpu->dedicated.used = FF_GPU_VMEM_SIZE_UNSET;
            }
            else
            {
                gpu->dedicated.total = device.recommendedMaxWorkingSetSize;
                gpu->dedicated.used = device.currentAllocatedSize;
            }
        }
        return NULL;
    }
    return "Metal API is not supported by this macOS version";
}
