// Copyright (c) 2023 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <process/process.hpp>

#include <atomic>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>

namespace ttldtor {
namespace process {

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

} // namespace process
} // namespace ttldtor

#ifdef WIN32

#    include <Windows.h>
#    include <processthreadsapi.h>
#    include <psapi.h>

namespace ttldtor {
namespace process {
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
} // namespace process
} // namespace ttldtor
#elif defined(__linux__)

#    include <sys/resource.h>

namespace ttldtor {
namespace process {

struct Parser {
    enum ParseResultType { KEY_NOT_FOUND, VALUE_NOT_FOUND, OK };

    struct ParseStatusResult {
        ParseResultType resultType;
        std::uint64_t value;
    };

    static ParseStatusResult parseStatus(const std::string &s, const std::string &key) noexcept {
        auto foundKeyPos = s.find(key);

        if (foundKeyPos != std::string::npos) {
            auto foundValuePos = s.find_first_of("0123456789", foundKeyPos + 6);

            if (foundValuePos != std::string::npos) {
                try {
                    return {OK, static_cast<std::uint64_t>(std::stoll(s.substr(foundValuePos)))};
                } catch (...) {
                    return {OK, 0};
                }
            } else {
                return {VALUE_NOT_FOUND, 0};
            }
        } else {
            return {KEY_NOT_FOUND, 0};
        }
    }
};

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

    if (is.fail()) {
        return 0ULL;
    }

    std::string s{};

    while (!std::getline(is, s).fail()) {
        auto result = Parser::parseStatus(s, "VmRSS:");

        if (result.resultType == Parser::KEY_NOT_FOUND) {
            continue;
        } else {
            return result.value * 1024;
        }
    }

    return 0LL;
}

std::uint64_t Process::getPrivateMemorySize() noexcept {
    std::ifstream is("/proc/self/status");

    if (is.fail()) {
        return 0ULL;
    }

    std::string s{};

    while (!std::getline(is, s).fail()) {
        auto result = Parser::parseStatus(s, "VmSize:");

        if (result.resultType == Parser::KEY_NOT_FOUND) {
            continue;
        } else {
            return result.value * 1024;
        }
    }

    return 0LL;
}
} // namespace process
} // namespace ttldtor
#elif defined(__APPLE__) && defined(__MACH__)
namespace ttldtor {
namespace process {
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
} // namespace process
} // namespace ttldtor
#else
namespace ttldtor {
namespace process {
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
}
}
}
#endif
