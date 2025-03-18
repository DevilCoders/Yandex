#pragma once

#include <functional>
#include <string>
#include <vector>

#define UNUSED(expr) do { (void) (expr); } while (0)

namespace yandex::maps::idl::utils {

/**
 * Returns given text in bold format for current console.
 */
std::string asConsoleBold(std::string text);

/**
 * Returns word's copy, but with capital first letter.
 */
std::string capitalizeWord(std::string word);

/**
 * Returns word's copy, but with lower-case first letter.
 */
std::string unCapitalizeWord(std::string word);

/**
 * Returns word's copy, but without initial "shared_ptr" part.
 */
std::string stripSharedPtr(std::string word);

/**
 * Returns word's copy, but without initial "scope" part.
 */
std::string stripScope(std::string word);

/**
 * Converts string in camel case into string in upper case with '_'
 * delimiters, e.g. "someString" or "SomeString"  into "SOME_STRING".
 */
std::string toUpperCase(std::string camelCaseString);

/**
 * Converts string with '_' delimiters into string in camel case, e.g.
 * "SOME_STRING", "some_string" or "Some_strInG" into "someString".
 */
std::string toCamelCase(
    const std::string& string,
    bool capitalizeFirstSymbol = false);

/**
 * Converts vector of "input" items into a vector of "output" items using
 * given converter functor.
 */
template <typename OutputItem, typename InputItem>
std::vector<OutputItem> convert(
    std::vector<InputItem> inputs,
    std::function<OutputItem(InputItem)> converter)
{
    std::vector<OutputItem> outputs;
    outputs.reserve(inputs.size());

    for (auto input : inputs) {
        outputs.push_back(converter(input));
    }

    return outputs;
}

/**
 * Same as above, but for converter as a function pointer (function pointer
 * can only be cast to std::function explicitly).
 */
template <typename OutputItem, typename InputItem>
std::vector<OutputItem> convert(
    std::vector<InputItem> inputs,
    OutputItem (*converter)(InputItem))
{
    return convert(inputs, std::function<OutputItem(InputItem)>(converter));
}

namespace internal {

template <typename Lambda, typename Result, typename FirstArgument, typename ...RemainingArguments>
FirstArgument FirstArgumentFunction(Result (Lambda::*)(FirstArgument, RemainingArguments...));
template <typename Lambda, typename Result, typename FirstArgument, typename ...RemainingArguments>
FirstArgument FirstArgumentFunction(Result (Lambda::*)(FirstArgument, RemainingArguments...) const);

template <typename Lambda, typename Result, typename ...Arguments>
Result ResultFunction(Result (Lambda::*)(Arguments...));
template <typename Lambda, typename Result, typename ...Arguments>
Result ResultFunction(Result (Lambda::*)(Arguments...) const);

} // namespace internal

template <typename Lambda>
struct LambdaTraits {
    using FirstArgument = decltype(internal::FirstArgumentFunction(&Lambda::operator()));
    using Result = decltype(internal::ResultFunction(&Lambda::operator()));
};

} // namespace yandex::maps::idl::utils
