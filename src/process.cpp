// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <process/process.hpp>

#include <atomic>
#include <chrono>
#include <cstring>
#include <fstream>
#include <string>
#include <type_traits>

TTLDTOR_PROCESS_BEGIN_NAMESPACE

template <typename... T> constexpr void ignore_unused(const T &...) {
}

constexpr inline auto is_constant_evaluated(bool default_value = false) noexcept -> bool {
#ifdef __cpp_lib_is_constant_evaluated
    ignore_unused(default_value);
    return std::is_constant_evaluated();
#else
    return default_value;
#endif
}

// Implementation of std::bit_cast for pre-C++20.
template <typename To, typename From>
constexpr To bit_cast(const From &from)
#if __cpp_concepts
    requires(sizeof(To) == sizeof(From))
#endif
{
#ifdef __cpp_lib_bit_cast
    if (is_constant_evaluated())
        return std::bit_cast<To>(from);
#endif
    auto to = To();
    // The cast suppresses a bogus -Wclass-memaccess on GCC.
    std::memcpy(static_cast<void *>(&to), &from, sizeof(to));
    return to;
}

TTLDTOR_PROCESS_END_NAMESPACE

#if defined(__linux__) || defined(__ANDROID__) || defined(__FreeBSD__)

#    include <sys/resource.h>

TTLDTOR_PROCESS_BEGIN_NAMESPACE

struct RUsageResult {
    std::chrono::milliseconds sysTime{};
    std::chrono::milliseconds userTime{};
    std::chrono::milliseconds totalTime{};

    explicit RUsageResult(const rusage &ru)
        : sysTime{static_cast<std::uint64_t>(ru.ru_stime.tv_sec) * 1000ULL +
                  static_cast<std::uint64_t>(ru.ru_stime.tv_usec) / 1000ULL},
          userTime{static_cast<std::uint64_t>(ru.ru_utime.tv_sec) * 1000ULL +
                   static_cast<std::uint64_t>(ru.ru_utime.tv_usec) / 1000ULL},
          totalTime{sysTime + userTime} {
    }
};

TTLDTOR_PROCESS_END_NAMESPACE

#endif

#ifdef _WIN32

#    include <Windows.h>
#    include <processthreadsapi.h>
#    include <psapi.h>

TTLDTOR_PROCESS_BEGIN_NAMESPACE

std::chrono::milliseconds Process::getKernelProcessorTime() noexcept {
    FILETIME creationTime{};
    FILETIME exitTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};
    auto h = GetCurrentProcess();

    GetProcessTimes(h, &creationTime, &exitTime, &kernelTime, &userTime);

    auto kernelTime2 =
        static_cast<std::uint64_t>(kernelTime.dwLowDateTime) |
        (static_cast<std::uint64_t>(kernelTime.dwHighDateTime) << (sizeof(kernelTime.dwLowDateTime) * 8ULL));

    return std::chrono::milliseconds(kernelTime2 / 10'000); // 100 nanoseconds time-points
}

std::chrono::milliseconds Process::getUserProcessorTime() noexcept {
    FILETIME creationTime{};
    FILETIME exitTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};
    auto h = GetCurrentProcess();

    GetProcessTimes(h, &creationTime, &exitTime, &kernelTime, &userTime);

    auto userTime2 = static_cast<std::uint64_t>(userTime.dwLowDateTime) |
                     (static_cast<std::uint64_t>(userTime.dwHighDateTime) << (sizeof(userTime.dwLowDateTime) * 8ULL));

    return std::chrono::milliseconds(userTime2 / 10'000); // 100 nanoseconds time-points
}

std::chrono::milliseconds Process::getTotalProcessorTime() noexcept {
    FILETIME creationTime{};
    FILETIME exitTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};
    auto h = GetCurrentProcess();

    GetProcessTimes(h, &creationTime, &exitTime, &kernelTime, &userTime);

    auto kernelTime2 =
        static_cast<std::uint64_t>(kernelTime.dwLowDateTime) |
        (static_cast<std::uint64_t>(kernelTime.dwHighDateTime) << (sizeof(kernelTime.dwLowDateTime) * 8ULL));
    auto userTime2 = static_cast<std::uint64_t>(userTime.dwLowDateTime) |
                     (static_cast<std::uint64_t>(userTime.dwHighDateTime) << (sizeof(userTime.dwLowDateTime) * 8ULL));

    return std::chrono::milliseconds((kernelTime2 + userTime2) / 10'000); // 100 nanoseconds time-points
}

std::uint64_t Process::getWorkingSetSize() noexcept {
    PROCESS_MEMORY_COUNTERS_EX processMemoryCountersEx{};
    auto h = GetCurrentProcess();

    GetProcessMemoryInfo(h, bit_cast<PROCESS_MEMORY_COUNTERS *>(&processMemoryCountersEx),
                         sizeof(processMemoryCountersEx));

    return static_cast<std::uint64_t>(processMemoryCountersEx.WorkingSetSize);
}

std::uint64_t Process::getPrivateMemorySize() noexcept {
    PROCESS_MEMORY_COUNTERS_EX processMemoryCountersEx{};
    auto h = GetCurrentProcess();

    GetProcessMemoryInfo(h, bit_cast<PROCESS_MEMORY_COUNTERS *>(&processMemoryCountersEx),
                         sizeof(processMemoryCountersEx));

    return static_cast<std::uint64_t>(processMemoryCountersEx.PrivateUsage);
}

TTLDTOR_PROCESS_END_NAMESPACE

#elif defined(__linux__) || defined(__ANDROID__)

#    include "LinuxStatusParser.hpp"

TTLDTOR_PROCESS_BEGIN_NAMESPACE

std::chrono::milliseconds Process::getKernelProcessorTime() noexcept {
    rusage ru{};

    getrusage(RUSAGE_SELF, &ru);

    return RUsageResult{ru}.sysTime;
}

std::chrono::milliseconds Process::getUserProcessorTime() noexcept {
    rusage ru{};

    getrusage(RUSAGE_SELF, &ru);

    return RUsageResult{ru}.userTime;
}

std::chrono::milliseconds Process::getTotalProcessorTime() noexcept {
    rusage ru{};

    getrusage(RUSAGE_SELF, &ru);

    return RUsageResult{ru}.totalTime;
}

std::uint64_t Process::getWorkingSetSize() noexcept {
    std::ifstream is("/proc/self/status");

    if (!is) {
        return 0;
    }

    std::string line;

    while (std::getline(is, line)) {
        const auto result = detail::parseLinuxStatusLine(line, "VmRSS:");

        if (result.status == detail::ParseStatus::KEY_NOT_FOUND) {
            continue;
        }

        if (result.status == detail::ParseStatus::OK) {
            return result.value;
        }

        return 0;
    }

    return 0;
}

std::uint64_t Process::getPrivateMemorySize() noexcept {
    std::ifstream is("/proc/self/status");

    if (!is) {
        return 0;
    }

    std::string line;

    while (std::getline(is, line)) {
        const auto result = detail::parseLinuxStatusLine(line, "VmSize:");

        if (result.status == detail::ParseStatus::KEY_NOT_FOUND) {
            continue;
        }

        if (result.status == detail::ParseStatus::OK) {
            return result.value;
        }

        return 0;
    }

    return 0;
}

TTLDTOR_PROCESS_END_NAMESPACE

#elif defined(__APPLE__) && defined(__MACH__)
#    include <unistd.h>
#    if __has_include(<libproc.h>)
#        include <libproc.h>
#    else
#        include <Availability.h>
#        include <sys/resource.h>
// Some SDKs are missing the libproc.h header
int proc_pid_rusage(int pid, int flavor, rusage_info_t *buffer) __OSX_AVAILABLE_STARTING(__MAC_10_9, __IPHONE_7_0);
#    endif
#    include <mach-o/dyld_images.h>
#    include <mach/mach.h>
#    include <mach/mach_time.h>
#    include <sys/sysctl.h>

TTLDTOR_PROCESS_BEGIN_NAMESPACE

struct ProcPidRUsageResult {
    static const std::uint64_t NSEC_TO_MSEC_RATIO = 1000000ULL;

    std::chrono::milliseconds sysTime{};
    std::chrono::milliseconds userTime{};
    std::chrono::milliseconds totalTime{};

    struct StaticData {
        mach_timebase_info_data_t timeBase;

        StaticData() {
            timeBase.numer = 0;
            timeBase.denom = 0;

            // Initialize time conversions.
            mach_timebase_info(&timeBase);
        }
    };

    static const StaticData staticData;

    explicit ProcPidRUsageResult(const rusage_info_v2 &rusage_info_data) {
        // std::cout << "Time Base: " << staticData.timeBase.numer << " / " << staticData.timeBase.denom << std::endl;

        sysTime = std::chrono::milliseconds(
            ((static_cast<std::uint64_t>(rusage_info_data.ri_system_time) * staticData.timeBase.numer) /
             staticData.timeBase.denom) /
            ProcPidRUsageResult::NSEC_TO_MSEC_RATIO);
        userTime = std::chrono::milliseconds(
            ((static_cast<std::uint64_t>(rusage_info_data.ri_user_time) * staticData.timeBase.numer) /
             staticData.timeBase.denom) /
            ProcPidRUsageResult::NSEC_TO_MSEC_RATIO);
        totalTime = sysTime + userTime;
    }
};

const ProcPidRUsageResult::StaticData ProcPidRUsageResult::staticData{};

std::chrono::milliseconds Process::getKernelProcessorTime() noexcept {
    rusage_info_v2 rusage_info_data{};
    auto status = proc_pid_rusage(getpid(), RUSAGE_INFO_V2, bit_cast<rusage_info_t *>(&rusage_info_data));

    return ProcPidRUsageResult{rusage_info_data}.sysTime;
}

std::chrono::milliseconds Process::getUserProcessorTime() noexcept {
    rusage_info_v2 rusage_info_data{};
    auto status = proc_pid_rusage(getpid(), RUSAGE_INFO_V2, bit_cast<rusage_info_t *>(&rusage_info_data));

    return ProcPidRUsageResult{rusage_info_data}.userTime;
}

std::chrono::milliseconds Process::getTotalProcessorTime() noexcept {
    rusage_info_v2 rusage_info_data{};
    auto status = proc_pid_rusage(getpid(), RUSAGE_INFO_V2, bit_cast<rusage_info_t *>(&rusage_info_data));

    return ProcPidRUsageResult{rusage_info_data}.totalTime;
}

std::uint64_t Process::getWorkingSetSize() noexcept {
    mach_task_basic_info info{};
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;

    auto result = task_info(mach_task_self(), MACH_TASK_BASIC_INFO, bit_cast<task_info_t>(&info), &infoCount);

    return static_cast<std::uint64_t>(info.resident_size);
}

std::uint64_t Process::getPrivateMemorySize() noexcept {
    mach_task_basic_info info{};
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;

    auto result = task_info(mach_task_self(), MACH_TASK_BASIC_INFO, bit_cast<task_info_t>(&info), &infoCount);

    return static_cast<std::uint64_t>(info.virtual_size);
}

TTLDTOR_PROCESS_END_NAMESPACE

#elif defined(__FreeBSD__)

#    include <errno.h>
#    include <sys/param.h>
#    include <sys/sysctl.h>
#    include <sys/types.h>
#    include <sys/user.h>
#    include <unistd.h>

TTLDTOR_PROCESS_BEGIN_NAMESPACE

bool getProcInfo(int pid, kinfo_proc &info) noexcept {
    const std::size_t MIB_SIZE = 4; // 6 - OpenBSD
    // MIB - Management Information Base
    int mib[MIB_SIZE] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()};
    std::size_t length = sizeof(kinfo_proc);

    if (sysctl(mib, MIB_SIZE, &info, &length, nullptr, 0) < 0) {
        return false;
    }

    return true;
}

std::chrono::milliseconds Process::getKernelProcessorTime() noexcept {
    kinfo_proc info{};

    if (getProcInfo(getpid(), info)) {
        return RUsageResult{info.ki_rusage}.sysTime;
    }

    return std::chrono::milliseconds(0);
}

std::chrono::milliseconds Process::getUserProcessorTime() noexcept {
    kinfo_proc info{};

    if (getProcInfo(getpid(), info)) {
        return RUsageResult{info.ki_rusage}.userTime;
    }

    return std::chrono::milliseconds(0);
}

std::chrono::milliseconds Process::getTotalProcessorTime() noexcept {
    kinfo_proc info{};

    if (getProcInfo(getpid(), info)) {
        return RUsageResult{info.ki_rusage}.totalTime;
    }

    return std::chrono::milliseconds(0);
}

std::uint64_t Process::getWorkingSetSize() noexcept {
    kinfo_proc info{};

    if (getProcInfo(getpid(), info)) {
        return static_cast<std::uint64_t>(info.ki_rssize) * getpagesize();
    }

    return 0ULL;
}

std::uint64_t Process::getPrivateMemorySize() noexcept {
    kinfo_proc info{};

    if (getProcInfo(getpid(), info)) {
        return static_cast<std::uint64_t>(info.ki_size);
    }

    return 0ULL;
}

TTLDTOR_PROCESS_END_NAMESPACE

#else

TTLDTOR_PROCESS_BEGIN_NAMESPACE

std::chrono::milliseconds Process::getKernelProcessorTime() noexcept {
    return std::chrono::milliseconds(0);
}

std::chrono::milliseconds Process::getUserProcessorTime() noexcept {
    return std::chrono::milliseconds(0);
}

std::chrono::milliseconds Process::getTotalProcessorTime() noexcept {
    return std::chrono::milliseconds(0);
}

std::uint64_t Process::getWorkingSetSize() noexcept {
    return 0ULL;
}

std::uint64_t Process::getPrivateMemorySize() noexcept {
    return 0ULL;
}

TTLDTOR_PROCESS_END_NAMESPACE

#endif
