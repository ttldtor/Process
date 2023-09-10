#pragma once

#ifdef _WINNT_
#    error Please, include windows.h or winnt.h after dxFeed Graal CXX API headers
#endif

#ifdef TTLDTOR_PROCESS_EXPORT
#    error TTLDTOR_PROCESS_EXPORT was previously defined
#endif

#if defined(TTLDTOR_PROCESS_USE_DLLS) && defined(_MSC_VER)
#    if defined(LIB_TTLDTOR_PROCESS_EXPORTS)
#        define TTLDTOR_PROCESS_EXPORT __declspec(dllexport)
#        define TTLDTOR_PROCESS_EXPORT_TEMPLATE_DECLARE
#        define TTLDTOR_PROCESS_EXPORT_TEMPLATE_DEFINE __declspec(dllexport)
#    else
#        define TTLDTOR_PROCESS_EXPORT __declspec(dllimport)
#        define TTLDTOR_PROCESS_EXPORT_TEMPLATE_DECLARE
#        define TTLDTOR_PROCESS_EXPORT_TEMPLATE_DEFINE __declspec(dllimport)
#    endif // defined(LIB_TTLDTOR_PROCESS)
#elif defined(TTLDTOR_PROCESS_USE_DLLS) && defined(LIB_TTLDTOR_PROCESS_EXPORTS)
#    define TTLDTOR_PROCESS_EXPORT __attribute__((visibility("default")))
#    define TTLDTOR_PROCESS_EXPORT_TEMPLATE_DECLARE __attribute__((visibility("default")))
#    define TTLDTOR_PROCESS_EXPORT_TEMPLATE_DEFINE
#else
#    define TTLDTOR_PROCESS_EXPORT
#    define TTLDTOR_PROCESS_EXPORT_TEMPLATE_DECLARE
#    define TTLDTOR_PROCESS_EXPORT_TEMPLATE_DEFINE
#endif

#include <chrono>
#include <cstdint>

namespace ttldtor::process {
struct TTLDTOR_PROCESS_EXPORT Process {

    static std::chrono::milliseconds getKernelProcessorTime() noexcept;
    static std::chrono::milliseconds getUserProcessorTime() noexcept;
    static std::chrono::milliseconds getTotalProcessorTime() noexcept;
};
} // namespace ttldtor::process