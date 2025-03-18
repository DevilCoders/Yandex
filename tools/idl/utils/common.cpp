#include <yandex/maps/idl/utils/common.h>

#include <yandex/maps/idl/utils/exception.h>

#include <cctype>
#include <cstddef>

namespace yandex::maps::idl::utils {

std::string asConsoleBold(std::string text)
{
#if defined(__GNUC__)
    return "\033[1m" + text + "\033[0m";
#else
    return text;
#endif
}

std::string capitalizeWord(std::string word)
{
    if (!word.empty()) {
        word[0] = std::toupper(word[0]);
    }
    return word;
}

std::string unCapitalizeWord(std::string word)
{
    if (!word.empty()) {
        word[0] = std::tolower(word[0]);
    }
    return word;
}

std::string stripSharedPtr(std::string word)
{
    static const std::string PREFIX = "std::shared_ptr<";
    static const std::string CONST_PREFIX = "const std::shared_ptr<";

    if (!word.empty() && word.back() == '>') {
        if (word.starts_with(PREFIX)) {
            return word.substr(PREFIX.size(), word.size() - PREFIX.size() - 1);
        } else if (word.starts_with(CONST_PREFIX)) {
            return word.substr(CONST_PREFIX.size(), word.size() - CONST_PREFIX.size() - 1);
        }
    }

    return word;
}

std::string stripScope(std::string word)
{
    static const std::string DELIMITERS = ":.";

    if (word.empty()) {
        return word;
    }

    auto lastPosition = word.find_last_of(DELIMITERS);

    // If not found, lastPosition will be equal to string::npos, which is -1.
    // We'll add 1 and get 0, which gives full string - as expected!
    return word.substr(lastPosition + 1);
}

std::string toUpperCase(std::string camelCaseString)
{
    if (camelCaseString.empty()) {
        return camelCaseString;
    }

    std::string upperCaseString;
    upperCaseString += char(std::toupper(camelCaseString[0]));

    for (std::size_t i = 1; i < camelCaseString.size(); ++i) {
        char c = camelCaseString[i];
        if (std::isupper(c)) {
            upperCaseString += '_';
            upperCaseString += c;
        } else {
            upperCaseString += std::toupper(c);
        }
    }

    return upperCaseString;
}

std::string toCamelCase(
    const std::string& string,
    bool capitalizeFirstSymbol)
{
    if (string.empty()) {
        return string;
    }

    std::string result;

    bool newWord = true;
    for(char c : string) {
        if (c == '_') {
            newWord = true;
        } else {
            result += newWord ? std::toupper(c) : std::tolower(c);
            newWord = false;
        }
    }

    if (!capitalizeFirstSymbol) {
        result[0] = std::tolower(result[0]);
    }
    return result;
}

} // namespace yandex::maps::idl::utils
