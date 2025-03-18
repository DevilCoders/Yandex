#pragma once

#include <exception>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include <yandex/maps/idl/utils/common.h>

namespace yandex {
namespace maps {
namespace idl {
namespace utils {

class Exception: public std::exception {
public:
    explicit Exception(const std::string& what = std::string())
        : what_(what)
    {
    }

    Exception(const Exception& copy) : what_(copy.what()) { }
    Exception(Exception&& /* source */) = default;
    virtual ~Exception() noexcept { }

    virtual const char* what() const noexcept
    {
        return what_.c_str();
    }

    /**
     * Cannot simply have Exception& operator<<(...) { ... return *this; },
     * because that will return exception of base type, and in general we
     * need to return exceptions of exact types (e.g. utils::UsageError).
     *
     * Also, cannot simply have
     * E&& operator<<(E&& e, ...) { ... return std::forward<E>(e); }, because
     * some compilers will think that part "stream << t;" inside this method
     * calls this same method, but with std::stringstream as E type parameter!
     */
    template <typename E, typename T>
    friend typename std::enable_if<
        std::is_base_of<
            Exception,
            typename std::remove_reference<E>::type>::value,
        E&&>::type operator<<(E&& e, const T& t)
    {
        std::stringstream stream;
        stream << t;
        e.what_ += stream.str();

        return std::forward<E>(e);
    }

private:
    std::string what_;
};

#define INTERNAL_ERROR(message) \
    throw ::yandex::maps::idl::utils::Exception() << \
        ("\n  " + ::yandex::maps::idl::utils::asConsoleBold("tools-idl-app internal error:") + "\n    ") << message;

#if defined(__GNUC__)
    #define LIKELY(x) __builtin_expect(static_cast<bool>(x), 1)
    #define UNLIKELY(x) __builtin_expect(static_cast<bool>(x), 0)
#else
    #define LIKELY(x) static_cast<bool>(x)
    #define UNLIKELY(x) static_cast<bool>(x)
#endif

// A helper macro, which simplifies checks like
//    if (query.points.empty()) { INTERNAL_ERROR("at least one point required"); }
#define REQUIRE(condition, message) \
    if (!LIKELY(condition)) { \
        INTERNAL_ERROR(message); \
    }

void installStackTracePrintingSignalHandler();

} // namespace utils
} // namespace idl
} // namespace maps
} // namespace yandex
