// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstdint>
#include <limits>
#include <string>

namespace org {
namespace ttldtor {
namespace process {
namespace detail {

enum class ParseStatus { KEY_NOT_FOUND, VALUE_NOT_FOUND, INVALID_VALUE, INVALID_UNIT, OUT_OF_RANGE, OK };

struct ParseStatusResult {
    ParseStatus status;
    std::uint64_t value;
};

inline ParseStatusResult parseLinuxStatusLine(const std::string &line, const std::string &key) noexcept {
    if (line.compare(0, key.size(), key) != 0) {
        return {ParseStatus::KEY_NOT_FOUND, 0};
    }

    auto pos = key.size();

    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) {
        ++pos;
    }

    if (pos == line.size()) {
        return {ParseStatus::VALUE_NOT_FOUND, 0};
    }

    if (line[pos] < '0' || line[pos] > '9') {
        return {ParseStatus::INVALID_VALUE, 0};
    }

    std::uint64_t value = 0;

    while (pos < line.size() && line[pos] >= '0' && line[pos] <= '9') {
        const auto digit = static_cast<std::uint64_t>(line[pos] - '0');

        if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10) {
            return {ParseStatus::OUT_OF_RANGE, 0};
        }

        value = value * 10 + digit;
        ++pos;
    }

    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) {
        ++pos;
    }

    // /proc/<pid>/status reports memory values in kB.
    if (line.compare(pos, 2, "kB") != 0) {
        return {ParseStatus::INVALID_UNIT, 0};
    }

    pos += 2;

    while (pos < line.size()) {
        const auto ch = line[pos];

        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
            return {ParseStatus::INVALID_UNIT, 0};
        }

        ++pos;
    }

    constexpr std::uint64_t KIBIBYTE = 1024;

    if (value > std::numeric_limits<std::uint64_t>::max() / KIBIBYTE) {
        return {ParseStatus::OUT_OF_RANGE, 0};
    }

    return {ParseStatus::OK, value * KIBIBYTE};
}

} // namespace detail
} // namespace process
} // namespace ttldtor
} // namespace org
