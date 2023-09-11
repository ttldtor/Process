// Copyright (c) 2023 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <process/process.hpp>

#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <atomic>

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

#ifdef WIN32

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
#elif defined(__linux__)
int parseLine(char *line) {
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char *p = line;
    while (*p < '0' || *p > '9')
        p++;
    line[i - 3] = '\0';
    i = atoi(p);
    return i;
}

struct Parser {
    enum ParseResultType { KEY_NOT_FOUND, VALUE_NOT_FOUND, OK };

    struct ParseResult {
        ParseResultType resultType;
        std::uint64_t value;
    };

    static ParseResult parse(const std::string &s, const std::string &key) noexcept {
        if (auto foundKeyPos = s.find(key); foundKeyPos != std::string::npos) {
            if (auto foundValuePos = s.find_first_of("0123456789", foundKeyPos + 6);
                foundValuePos != std::string::npos) {
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



static unsigned long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;

void init() {
    FILE *file = fopen("/proc/stat", "r");
    fscanf(file, "cpu %llu %llu %llu %llu", &lastTotalUser, &lastTotalUserLow, &lastTotalSys, &lastTotalIdle);
    fclose(file);
}

namespace ttldtor::process {
/*TODO: implement
 * https://github.com/dotnet/runtime/blob/de0ab156194eb64deae0e1018db9a58f7b02f4a3/src/libraries/System.Diagnostics.Process/src/System/Diagnostics/Process.Unix.cs#L817
 * https://github.com/dotnet/runtime/blob/de0ab156194eb64deae0e1018db9a58f7b02f4a3/src/libraries/System.Diagnostics.Process/src/System/Diagnostics/Process.Linux.cs#L130
 */

std::chrono::milliseconds Process::getKernelProcessorTime() noexcept {
    return std::chrono::milliseconds(0);
}

std::chrono::milliseconds Process::getUserProcessorTime() noexcept {
    return std::chrono::milliseconds(0);
}

std::chrono::milliseconds Process::getTotalProcessorTime() noexcept {
    init();

    // std::cout << (lastTotalUser + lastTotalSys) << std::endl;

    return std::chrono::milliseconds((lastTotalUser + lastTotalSys) * 10);
}

std::uint64_t Process::getWorkingSetSize() noexcept {
    std::ifstream is("/proc/self/status");

    if (is.fail()) {
        return 0ULL;
    }

    std::string s{};

    while (!std::getline(is, s).fail()) {
        auto result = Parser::parse(s, "VmRSS:");

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
        auto result = Parser::parse(s, "VmSize:");

        if (result.resultType == Parser::KEY_NOT_FOUND) {
            continue;
        } else {
            return result.value * 1024;
        }
    }

    return 0LL;
}
} // namespace ttldtor::process
#elif defined(__APPLE__) && defined(__MACH__)
namespace ttldtor::process {
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
} // namespace ttldtor::process
#else
namespace ttldtor::process {
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
} // namespace ttldtor::process
#endif
