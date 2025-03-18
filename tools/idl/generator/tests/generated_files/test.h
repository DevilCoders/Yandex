#pragma once

#include <yandex/maps/runtime/assert.h>
#include <yandex/maps/runtime/bindings/platform.h>
#include <yandex/maps/runtime/bindings/traits.h>
#include <yandex/maps/runtime/platform_holder.h>
#include <yandex/maps/runtime/serialization/chrono.h>
#include <yandex/maps/runtime/serialization/ptr.h>
#include <yandex/maps/runtime/time.h>

#include <boost/any.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/variant.hpp>

#include <functional>
#include <memory>
#include <string>

namespace test {
namespace docs {

struct YANDEX_EXPORT Struct {

    Struct();
    explicit Struct(
        int i);
    
    Struct(const Struct&) = default;
    Struct(Struct&&) = default;
    
    Struct& operator=(const Struct&) = default;
    Struct& operator=(Struct&&) = default;

    int i;
};

using Variant = boost::variant<
    int,
    float>;

enum class TestError {

    Server,

    Client
};


struct YANDEX_EXPORT Response {

    Response();
    
    Response(
        int i,
        float f);
    
    Response(const Response& other) = default;
    Response(Response&&) = default;
    
    Response& operator=(const Response& other) = default;
    Response& operator=(Response&&) = default;

    int i;

    float f;
};

class YANDEX_EXPORT OptionsStructure {
public:

    OptionsStructure() = default;
    OptionsStructure(
        const std::string& empty,
        const std::string& filled);
    
    OptionsStructure(const OptionsStructure&) = default;
    OptionsStructure(OptionsStructure&&) = default;
    
    OptionsStructure& operator=(const OptionsStructure&) = default;
    OptionsStructure& operator=(OptionsStructure&&) = default;

    const std::string& empty() const { return empty_; }
    OptionsStructure& setEmpty(const std::string& empty)
    {
        empty_ = empty;
        return *this;
    }

    const std::string& filled() const { return filled_; }
    OptionsStructure& setFilled(const std::string& filled)
    {
        filled_ = filled;
        return *this;
    }

    template <typename Archive>
    void serialize(
        Archive& ar,
        const unsigned int /* version */)
    {
        ar & boost::serialization::make_nvp("empty", empty_);
        ar & boost::serialization::make_nvp("filled", filled_);
    }
    

private:
    std::string empty_;
    
    std::string filled_{ "default value" };
};

struct YANDEX_EXPORT DefaultValue {

    DefaultValue();
    explicit DefaultValue(
        const std::string& filled);
    
    DefaultValue(const DefaultValue&) = default;
    DefaultValue(DefaultValue&&) = default;
    
    DefaultValue& operator=(const DefaultValue&) = default;
    DefaultValue& operator=(DefaultValue&&) = default;

    std::string filled{ "default value" };
};

struct YANDEX_EXPORT CombinedValues {

    CombinedValues();
    CombinedValues(
        const std::string& empty,
        const std::string& filled);
    
    CombinedValues(const CombinedValues&) = default;
    CombinedValues(CombinedValues&&) = default;
    
    CombinedValues& operator=(const CombinedValues&) = default;
    CombinedValues& operator=(CombinedValues&&) = default;

    std::string empty;

    std::string filled{ "default value" };
};

struct YANDEX_EXPORT DefaultTimeintervalValue {

    DefaultTimeintervalValue();
    DefaultTimeintervalValue(
        ::yandex::maps::runtime::TimeInterval empty,
        ::yandex::maps::runtime::TimeInterval filled);
    
    DefaultTimeintervalValue(const DefaultTimeintervalValue&) = default;
    DefaultTimeintervalValue(DefaultTimeintervalValue&&) = default;
    
    DefaultTimeintervalValue& operator=(const DefaultTimeintervalValue&) = default;
    DefaultTimeintervalValue& operator=(DefaultTimeintervalValue&&) = default;

    ::yandex::maps::runtime::TimeInterval empty;

    ::yandex::maps::runtime::TimeInterval filled{ 300 };
};

class YANDEX_EXPORT Interface : public ::yandex::maps::runtime::PlatformHolder<Interface> {
public:
    virtual ~Interface()
    {
    }

    virtual void method(
        int intValue,
        float floatValue,
        const ::test::docs::Struct& someStruct,
        ::test::docs::Variant& andVariant) = 0;
};

using OnResponse = std::function<void(
    const std::shared_ptr<::test::docs::Response>& response)>;

using OnError = std::function<void(
    ::test::docs::TestError error)>;

class YANDEX_EXPORT InterfaceWithDocs {
public:
    virtual ~InterfaceWithDocs()
    {
    }

    /**
     * Link to ::test::docs::Struct, to ::test::docs::Variant::f, and some
     * unsupported tag {@some.unsupported.tag}.
     *
     * More links after separator: ::test::docs::Struct,
     * ::test::docs::Interface::method(int, float, const
     * ::test::docs::Struct&, ::test::docs::Variant&), and link to self:
     * methodWithDocs(::test::docs::Interface*, const ::test::docs::Struct&,
     * const ::test::docs::OnResponse&, const ::test::docs::OnError&).
     *
     * @param i - ::test::docs::Interface, does something important
     * @param s - some struct
     *
     * @return true if successful, false - otherwise
     */
    virtual bool methodWithDocs(
        ::test::docs::Interface* i,
        const ::test::docs::Struct& s,
        const ::test::docs::OnResponse& onResponse,
        const ::test::docs::OnError& onError) = 0;
};

#ifdef BUILDING_FOR_TARGET
YANDEX_EXPORT boost::any createPlatform(const std::shared_ptr<::test::docs::Interface>& interface);
#else
inline boost::any createPlatform(const std::shared_ptr<::test::docs::Interface>& /* interface */)
{
    ASSERT(false);
    return nullptr;
}
#endif

} // namespace docs
} // namespace test

namespace boost {
namespace serialization {

template <typename Archive>
void serialize(
    Archive& ar,
    ::test::docs::Struct& obj,
    const unsigned int /* version */)
{
    ar & boost::serialization::make_nvp("i", obj.i);
}


template <typename Archive>
void serialize(
    Archive& ar,
    ::test::docs::Response& obj,
    const unsigned int /* version */)
{
    ar & boost::serialization::make_nvp("i", obj.i);
    ar & boost::serialization::make_nvp("f", obj.f);
}


template <typename Archive>
void serialize(
    Archive& ar,
    ::test::docs::DefaultValue& obj,
    const unsigned int /* version */)
{
    ar & boost::serialization::make_nvp("filled", obj.filled);
}


template <typename Archive>
void serialize(
    Archive& ar,
    ::test::docs::CombinedValues& obj,
    const unsigned int /* version */)
{
    ar & boost::serialization::make_nvp("empty", obj.empty);
    ar & boost::serialization::make_nvp("filled", obj.filled);
}


template <typename Archive>
void serialize(
    Archive& ar,
    ::test::docs::DefaultTimeintervalValue& obj,
    const unsigned int /* version */)
{
    ar & boost::serialization::make_nvp("empty", obj.empty);
    ar & boost::serialization::make_nvp("filled", obj.filled);
}


} // namespace serialization
} // namespace boost

namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {

template <>
struct BindingTraits<::test::docs::Struct> : public BaseBindingTraits {
    static constexpr bool isStruct = true;
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/Struct";
    static constexpr const char* const objectiveCName = "YTDStruct";
    static constexpr const char* const cppName = "test::docs::Struct";
};

template <>
struct BindingTraits<::test::docs::Variant> : public BaseBindingTraits {
    static constexpr bool isVariant = true;
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/Variant";
    static constexpr const char* const objectiveCName = "YTDVariant";
    static constexpr const char* const cppName = "test::docs::Variant";
};

template <>
struct BindingTraits<::test::docs::TestError> : public BaseBindingTraits {
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/TestError";
    static constexpr const char* const objectiveCName = "YTDTestError";
    static constexpr const char* const cppName = "test::docs::TestError";
};

template <>
struct BindingTraits<::test::docs::Response> : public BaseBindingTraits {
    static constexpr bool isStruct = true;
    static constexpr bool isBridged = true;
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/Response";
    static constexpr const char* const objectiveCName = "YTDResponse";
    static constexpr const char* const cppName = "test::docs::Response";
};

template <>
struct BindingTraits<::test::docs::OptionsStructure> : public BaseBindingTraits {
    static constexpr bool isStruct = true;
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/OptionsStructure";
    static constexpr const char* const objectiveCName = "YTDOptionsStructure";
    static constexpr const char* const cppName = "test::docs::OptionsStructure";
};

template <>
struct BindingTraits<::test::docs::DefaultValue> : public BaseBindingTraits {
    static constexpr bool isStruct = true;
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/DefaultValue";
    static constexpr const char* const objectiveCName = "YTDDefaultValue";
    static constexpr const char* const cppName = "test::docs::DefaultValue";
};

template <>
struct BindingTraits<::test::docs::CombinedValues> : public BaseBindingTraits {
    static constexpr bool isStruct = true;
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/CombinedValues";
    static constexpr const char* const objectiveCName = "YTDCombinedValues";
    static constexpr const char* const cppName = "test::docs::CombinedValues";
};

template <>
struct BindingTraits<::test::docs::DefaultTimeintervalValue> : public BaseBindingTraits {
    static constexpr bool isStruct = true;
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/DefaultTimeintervalValue";
    static constexpr const char* const objectiveCName = "YTDDefaultTimeintervalValue";
    static constexpr const char* const cppName = "test::docs::DefaultTimeintervalValue";
};

template <>
struct BindingTraits<::test::docs::Interface> : public BaseBindingTraits {
    using TopMostBaseType = ::test::docs::Interface;
    static constexpr bool isWeakInterface = true;
    static constexpr const char* const javaBindingUndecoratedName = "ru/test/docs/internal/JavaInterfaceBinding";
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/JavaInterface";
    static constexpr const char* const objectiveCName = "YTDObjcInterface";
    static constexpr const char* const cppName = "test::docs::Interface";
};

template <>
struct BindingTraits<::test::docs::InterfaceWithDocs> : public BaseBindingTraits {
    using TopMostBaseType = ::test::docs::InterfaceWithDocs;
    static constexpr bool isStrongInterface = true;
    static constexpr const char* const javaBindingUndecoratedName = "ru/test/docs/internal/InterfaceWithDocsBinding";
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/InterfaceWithDocs";
    static constexpr const char* const objectiveCName = "YTDInterfaceWithDocs";
    static constexpr const char* const cppName = "test::docs::InterfaceWithDocs";
};

} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex
