// Copyright (c) 2023 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <process/process.hpp>

#include <chrono>
#include <cstring>
#include <iostream>
#include <type_traits>

namespace ttldtor::process {

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

} // namespace ttldtor::process

#if WIN32

#    include <Windows.h>
#    include <processthreadsapi.h>
#    include <psapi.h>

namespace ttldtor::process {
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
} // namespace ttldtor::process

#endif
