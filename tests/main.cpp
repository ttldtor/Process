#include <doctest/doctest.h>

#include <process/process.hpp>

using namespace org::ttldtor::process;

TEST_CASE("Process::getPrivateMemorySize must return a value greater than 0.") {
    const auto size = Process::getPrivateMemorySize();
    CHECK((size > 0));
}

#if defined(__linux__) || defined(__ANDROID__)

#    include "LinuxStatusParser.hpp"

TEST_CASE("The parser must correctly handle 'VmRSS' [Linux]") {
    using detail::parseLinuxStatusLine;
    using detail::ParseStatus;

    const auto valid = parseLinuxStatusLine("VmRSS:\t123 kB", "VmRSS:");
    CHECK_EQ(valid.status, ParseStatus::OK);
    CHECK_EQ(valid.value, 123 * 1024);

    CHECK_EQ(parseLinuxStatusLine("Name:\tVmRSS: 42", "VmRSS:").status, ParseStatus::KEY_NOT_FOUND);
    CHECK_EQ(parseLinuxStatusLine("VmRSS:\t-123 kB", "VmRSS:").status, ParseStatus::INVALID_VALUE);
    CHECK_EQ(parseLinuxStatusLine("VmRSS:\tabc kB", "VmRSS:").status, ParseStatus::INVALID_VALUE);
    CHECK_EQ(parseLinuxStatusLine("VmRSS:\t123 MB", "VmRSS:").status, ParseStatus::INVALID_UNIT);
    CHECK_EQ(parseLinuxStatusLine("VmRSS:", "VmRSS:").status, ParseStatus::VALUE_NOT_FOUND);
}

#endif
