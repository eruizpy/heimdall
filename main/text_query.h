#ifndef TEXT_QUERY_H_
#define TEXT_QUERY_H_

// Text query sanitizing (spec 001-text-query). Pure logic, testable on the host.

#include <string>

// Protocol::SendWakeWordDetected() builds JSON by concatenation, so drop characters that would
// break it (quotes, backslashes, control characters) and trim surrounding spaces. UTF-8 is kept.
inline std::string SanitizeTextQuery(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (unsigned char c : text) {
        if (c >= 0x20 && c != '"' && c != '\\') {
            out += static_cast<char>(c);
        }
    }
    size_t begin = out.find_first_not_of(' ');
    if (begin == std::string::npos) {
        return "";
    }
    size_t end = out.find_last_not_of(' ');
    return out.substr(begin, end - begin + 1);
}

#endif  // TEXT_QUERY_H_
