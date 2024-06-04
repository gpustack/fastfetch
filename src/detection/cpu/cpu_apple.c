#include "cpu.h"
#include "common/sysctl.h"
#include "detection/temps/temps_apple.h"
#include "util/stringUtils.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/mach_host.h>
#include <unistd.h>

static double detectCpuTemp(const FFstrbuf* cpuName)
{
    double result = 0;

    const char* error = NULL;
    if (ffStrbufStartsWithS(cpuName, "Apple M"))
    {
        switch (strtol(cpuName->chars + strlen("Apple M"), NULL, 10))
        {
            case 1: error = ffDetectSmcTemps(FF_TEMP_CPU_M1X, &result); break;
            case 2: error = ffDetectSmcTemps(FF_TEMP_CPU_M2X, &result); break;
            case 3: error = ffDetectSmcTemps(FF_TEMP_CPU_M3X, &result); break;
            default: error = "Unsupported Apple Silicon CPU";
        }
    }
    else // PPC?
        error = ffDetectSmcTemps(FF_TEMP_CPU_X64, &result);

    if(error)
        return FF_CPU_TEMP_UNSET;

    return result;
}

#ifdef __aarch64__
#include "util/apple/cf_helpers.h"

#include <IOKit/IOKitLib.h>

static const char* detectFrequency(FFCPUResult* cpu)
{
    // https://github.com/giampaolo/psutil/pull/2222/files

    FF_IOOBJECT_AUTO_RELEASE io_registry_entry_t entryDevice = IOServiceGetMatchingService(MACH_PORT_NULL, IOServiceNameMatching("pmgr"));
    if (!entryDevice)
        return "IOServiceGetMatchingServices() failed";

    if (!IOObjectConformsTo(entryDevice, "AppleARMIODevice"))
        return "\"pmgr\" should conform to \"AppleARMIODevice\"";

    FF_CFTYPE_AUTO_RELEASE CFMutableDictionaryRef properties = NULL;
    if (IORegistryEntryCreateCFProperties(entryDevice, &properties, kCFAllocatorDefault, kNilOptions) != kIOReturnSuccess)
        return "IORegistryEntryCreateCFProperties() failed";

    uint32_t pMin, eMin, aMax, pCoreLength;
    if (ffCfDictGetData(properties, CFSTR("voltage-states5-sram"), 0, 4, (uint8_t*) &pMin, &pCoreLength) != NULL) // pCore
        return "\"voltage-states5-sram\" in \"pmgr\" is not found";
    if (ffCfDictGetData(properties, CFSTR("voltage-states1-sram"), 0, 4, (uint8_t*) &eMin, NULL) != NULL) // eCore
        return "\"voltage-states1-sram\" in \"pmgr\" is not found";

    cpu->frequencyMin = (pMin < eMin ? pMin : eMin) / (1000.0 * 1000 * 1000);

    if (pCoreLength >= 8)
    {
        ffCfDictGetData(properties, CFSTR("voltage-states5-sram"), pCoreLength - 8, 4, (uint8_t*) &aMax, NULL);
        cpu->frequencyMax = aMax / (1000.0 * 1000 * 1000);
    }

    return NULL;
}
#else
static const char* detectFrequency(FFCPUResult* cpu)
{
    cpu->frequencyBase = ffSysctlGetInt64("hw.cpufrequency", 0) / 1000.0 / 1000.0 / 1000.0;
    cpu->frequencyMin = ffSysctlGetInt64("hw.cpufrequency_min", 0) / 1000.0 / 1000.0 / 1000.0;
    cpu->frequencyMax = ffSysctlGetInt64("hw.cpufrequency_max", 0) / 1000.0 / 1000.0 / 1000.0;
    if(cpu->frequencyBase != cpu->frequencyBase)
    {
        unsigned current = 0;
        size_t size = sizeof(current);
        if (sysctl((int[]){ CTL_HW, HW_CPU_FREQ }, 2, &current, &size, NULL, 0) == 0)
            cpu->frequencyBase = (double) current / 1000.0 / 1000.0 / 1000.0;
    }
    return NULL;
}
#endif

static const char* detectCoreCount(FFCPUResult* cpu)
{
    uint32_t nPerfLevels = (uint32_t) ffSysctlGetInt("hw.nperflevels", 0);
    if (nPerfLevels <= 0) return "sysctl(hw.nperflevels) failed";

    char sysctlKey[] = "hw.perflevelN.logicalcpu";
    if (nPerfLevels > sizeof(cpu->coreTypes) / sizeof(cpu->coreTypes[0]))
        nPerfLevels = sizeof(cpu->coreTypes) / sizeof(cpu->coreTypes[0]);
    for (uint32_t i = 0; i < nPerfLevels; ++i)
    {
        sysctlKey[strlen("hw.perflevel")] = (char) ('0' + i);
        cpu->coreTypes[i] = (FFCPUCore) {
            .freq = nPerfLevels - i,
            .count = (uint32_t) ffSysctlGetInt(sysctlKey, 0),
        };
    }
    return NULL;
}

static double detectCoreUtilizationRate()
{
    double cpuUsage = FF_CPU_UTILIZATION_RATE_UNSET;

    natural_t cpuCount;
    processor_info_array_t cpuInfo;
    processor_info_array_t prevCpuInfo = NULL;
    mach_msg_type_number_t numCPUInfo;
    mach_msg_type_number_t numPrevCpuInfo = 0;
    host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &cpuCount, (processor_info_array_t *)&cpuInfo, &numCPUInfo);

    sleep(1);

    prevCpuInfo = cpuInfo;
    numPrevCpuInfo = numCPUInfo;
    host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &cpuCount, (processor_info_array_t *)&cpuInfo, &numCPUInfo);

    float totalInUse = 0, totalTotal = 0;
    for (unsigned i = 0; i < cpuCount; i++)
    {
        float userTime = cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_USER] - prevCpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_USER];
        float systemTime = cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_SYSTEM] - prevCpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_SYSTEM];
        float niceTime = cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_NICE] - prevCpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_NICE];
        float idleTime = cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_IDLE] - prevCpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_IDLE];

        float inUse = userTime + systemTime + niceTime;
        float total = inUse + idleTime;

        totalInUse += inUse;
        totalTotal += total;
    }

    cpuUsage = (totalInUse / totalTotal) * 100;

    if (prevCpuInfo)
        vm_deallocate(mach_task_self(), (vm_address_t)prevCpuInfo, numPrevCpuInfo * sizeof(integer_t));

    if (cpuInfo)
        vm_deallocate(mach_task_self(), (vm_address_t)cpuInfo, numCPUInfo * sizeof(integer_t));

    return cpuUsage;
}

const char* ffDetectCPUImpl(const FFCPUOptions* options, FFCPUResult* cpu)
{
    if (ffSysctlGetString("machdep.cpu.brand_string", &cpu->name) != NULL)
        return "sysctlbyname(machdep.cpu.brand_string) failed";

    ffSysctlGetString("machdep.cpu.vendor", &cpu->vendor);
    if (cpu->vendor.length == 0 && ffStrbufStartsWithS(&cpu->name, "Apple "))
        ffStrbufAppendS(&cpu->vendor, "Apple");

    cpu->coresPhysical = (uint16_t) ffSysctlGetInt("hw.physicalcpu_max", 1);
    if(cpu->coresPhysical == 1)
        cpu->coresPhysical = (uint16_t) ffSysctlGetInt("hw.physicalcpu", 1);

    cpu->coresLogical = (uint16_t) ffSysctlGetInt("hw.logicalcpu_max", 1);
    if(cpu->coresLogical == 1)
        cpu->coresLogical = (uint16_t) ffSysctlGetInt("hw.ncpu", 1);

    cpu->coresOnline = (uint16_t) ffSysctlGetInt("hw.logicalcpu", 1);
    if(cpu->coresOnline == 1)
        cpu->coresOnline = (uint16_t) ffSysctlGetInt("hw.activecpu", 1);

    detectFrequency(cpu);
    if (options->showPeCoreCount) detectCoreCount(cpu);

    cpu->temperature = options->temp ? detectCpuTemp(&cpu->name) : FF_CPU_TEMP_UNSET;

    cpu->coresUtilizationRate = detectCoreUtilizationRate();

    return NULL;
}
