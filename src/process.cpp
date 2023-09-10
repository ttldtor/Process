#include <process/process.hpp>

#include <chrono>
#include <iostream>

void hello() {
    std::cout << "Hello, World!" << std::endl;
}

#if WIN32

#    include <Windows.h>
#    include <processthreadsapi.h>

namespace ttldtor::process {
std::chrono::milliseconds Process::getKernelProcessorTime() noexcept {
    auto h = GetCurrentProcess();
    FILETIME creationTime{};
    FILETIME exitTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};

    GetProcessTimes(h, &creationTime, &exitTime, &kernelTime, &userTime);

    auto kernelTime2 =
        static_cast<std::uint64_t>(kernelTime.dwLowDateTime) |
        (static_cast<std::uint64_t>(kernelTime.dwHighDateTime) << (sizeof(kernelTime.dwLowDateTime) * 8ULL));

    return std::chrono::milliseconds(kernelTime2 / 10'000); // 100 nanoseconds time-points
}

std::chrono::milliseconds Process::getUserProcessorTime() noexcept {
    auto h = GetCurrentProcess();
    FILETIME creationTime{};
    FILETIME exitTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};

    GetProcessTimes(h, &creationTime, &exitTime, &kernelTime, &userTime);

    auto userTime2 = static_cast<std::uint64_t>(userTime.dwLowDateTime) |
                     (static_cast<std::uint64_t>(userTime.dwHighDateTime) << (sizeof(userTime.dwLowDateTime) * 8ULL));

    return std::chrono::milliseconds(userTime2 / 10'000); // 100 nanoseconds time-points
}

std::chrono::milliseconds Process::getTotalProcessorTime() noexcept {
    auto h = GetCurrentProcess();
    FILETIME creationTime{};
    FILETIME exitTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};

    GetProcessTimes(h, &creationTime, &exitTime, &kernelTime, &userTime);

    auto kernelTime2 =
        static_cast<std::uint64_t>(kernelTime.dwLowDateTime) |
        (static_cast<std::uint64_t>(kernelTime.dwHighDateTime) << (sizeof(kernelTime.dwLowDateTime) * 8ULL));
    auto userTime2 = static_cast<std::uint64_t>(userTime.dwLowDateTime) |
                     (static_cast<std::uint64_t>(userTime.dwHighDateTime) << (sizeof(userTime.dwLowDateTime) * 8ULL));

    return std::chrono::milliseconds((kernelTime2 + userTime2) / 10'000); // 100 nanoseconds time-points
}
} // namespace ttldtor::process

#endif
