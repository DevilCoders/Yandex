#pragma once

#include <yandex/maps/runtime/bindings/platform.h>
#include <yandex/maps/runtime/bindings/traits.h>
#include <yandex/maps/runtime/serialization/ptr.h>

#include <boost/optional.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/optional.hpp>

#include <functional>
#include <memory>

namespace test {
namespace docs {

/// @cond EXCLUDE
/**
 *
 * This struct should be excluded from documentation.
 */
struct YANDEX_EXPORT VeryPrivate {

    VeryPrivate();
    
    VeryPrivate(
        int regularField,
        float oneMoreRegularField);
    
    VeryPrivate(const VeryPrivate& other) = default;
    VeryPrivate(VeryPrivate&&) = default;
    
    VeryPrivate& operator=(const VeryPrivate& other) = default;
    VeryPrivate& operator=(VeryPrivate&&) = default;

    int regularField;

    float oneMoreRegularField;
};
/// @endcond

/**
 *
 * This struct should be excluded from documentation.
 */
class YANDEX_EXPORT SuchHidden {
public:

    SuchHidden() = default;
    SuchHidden(
        float regularField,
        int oneMoreRegularField,
        float twoMoreRegularFields);
    
    SuchHidden(const SuchHidden&) = default;
    SuchHidden(SuchHidden&&) = default;
    
    SuchHidden& operator=(const SuchHidden&) = default;
    SuchHidden& operator=(SuchHidden&&) = default;

    float regularField() const { return regularField_; }
    SuchHidden& setRegularField(float regularField)
    {
        regularField_ = regularField;
        return *this;
    }

    int oneMoreRegularField() const { return oneMoreRegularField_; }
    SuchHidden& setOneMoreRegularField(int oneMoreRegularField)
    {
        oneMoreRegularField_ = oneMoreRegularField;
        return *this;
    }

    float twoMoreRegularFields() const { return twoMoreRegularFields_; }
    SuchHidden& setTwoMoreRegularFields(float twoMoreRegularFields)
    {
        twoMoreRegularFields_ = twoMoreRegularFields;
        return *this;
    }

    template <typename Archive>
    void serialize(
        Archive& ar,
        const unsigned int /* version */)
    {
        ar & boost::serialization::make_nvp("regularField", regularField_);
        ar & boost::serialization::make_nvp("oneMoreRegularField", oneMoreRegularField_);
        ar & boost::serialization::make_nvp("twoMoreRegularFields", twoMoreRegularFields_);
    }
    

private:
    float regularField_;
    
    int oneMoreRegularField_;
    
    float twoMoreRegularFields_;
};

/**
 *
 * This struct should be excluded from documentation.
 */
class YANDEX_EXPORT SuchOptions {
public:

    SuchOptions() = default;
    explicit SuchOptions(
        const std::unique_ptr<::test::docs::TooInternal>& interfaceField);
    
    SuchOptions(const SuchOptions&) = default;
    SuchOptions(SuchOptions&&) = default;
    
    SuchOptions& operator=(const SuchOptions&) = default;
    SuchOptions& operator=(SuchOptions&&) = default;

    const std::unique_ptr<::test::docs::TooInternal>& interfaceField() const { return interfaceField_; }
    SuchOptions& setInterfaceField(const std::unique_ptr<::test::docs::TooInternal>& interfaceField)
    {
        interfaceField_ = interfaceField;
        return *this;
    }

private:
    std::unique_ptr<::test::docs::TooInternal> interfaceField_;
};

/// @cond EXCLUDE
/**
 *
 * This listener should be excluded from documentation.
 */
using FirstCallback = std::function<void(
    const std::shared_ptr<::test::docs::VeryPrivate>& muchClassified)>;
/// @endcond

/// @cond EXCLUDE
/**
 *
 * This interface should be excluded from documentation.
 */
class YANDEX_EXPORT TooInternal {
public:
    virtual ~TooInternal()
    {
    }

    virtual bool regularMethod(
        const ::test::docs::SuchHidden& muchPrivate,
        const std::shared_ptr<::test::docs::VeryPrivate>& soInternal) = 0;
};
/// @endcond

struct YANDEX_EXPORT VeryOpen {

    VeryOpen();
    
    VeryOpen(
        int regularField,
        const boost::optional<bool>& hiddenSwitch);
    
    VeryOpen(const VeryOpen& other) = default;
    VeryOpen(VeryOpen&&) = default;
    
    VeryOpen& operator=(const VeryOpen& other) = default;
    VeryOpen& operator=(VeryOpen&&) = default;

    int regularField;

    /**
     *
     * Only this field should be excluded from docs
     *
     *
     */
    boost::optional<bool> hiddenSwitch;
};

struct YANDEX_EXPORT MuchUnprotected {

    MuchUnprotected();
    
    MuchUnprotected(
        float regularField,
        bool oneMoreRegularField,
        const boost::optional<int>& hiddenField);
    
    MuchUnprotected(const MuchUnprotected& other) = default;
    MuchUnprotected(MuchUnprotected&&) = default;
    
    MuchUnprotected& operator=(const MuchUnprotected& other) = default;
    MuchUnprotected& operator=(MuchUnprotected&&) = default;

    float regularField;

    bool oneMoreRegularField;

    /**
     *
     * Only this field should be excluded from docs
     *
     *
     */
    boost::optional<int> hiddenField;
};

using CantMarkMethodsAsInternalHereYet = std::function<void(
    const std::shared_ptr<::test::docs::VeryOpen>& knownStructure)>;

using OnEmpty = std::function<void()>;

using OnSuccess = std::function<void()>;

using OnError = std::function<void(
    int error)>;

using OnCallback = std::function<void(
    int i)>;

class YANDEX_EXPORT TooExternal {
public:
    virtual ~TooExternal()
    {
    }

    virtual bool regularMethod(
        const std::shared_ptr<::test::docs::MuchUnprotected>& structure,
        const ::test::docs::CantMarkMethodsAsInternalHereYet& cantMarkMethodsAsInternalHereYet) = 0;

    /// @cond EXCLUDE
    /**
     *
     * Only this method should be excluded from docs
     */
    virtual void hiddenMethod(
        const ::test::docs::SuchHidden& structure,
        const ::test::docs::FirstCallback& firstCallback) = 0;
    /// @endcond
};

struct YANDEX_EXPORT WithInternalEnum {

    enum class InternalEnum {
    
        A,
    
        B,
    
        C
    };
    

    WithInternalEnum();
    
    explicit WithInternalEnum(
        ::test::docs::WithInternalEnum::InternalEnum e);
    
    WithInternalEnum(const WithInternalEnum& other) = default;
    WithInternalEnum(WithInternalEnum&&) = default;
    
    WithInternalEnum& operator=(const WithInternalEnum& other) = default;
    WithInternalEnum& operator=(WithInternalEnum&&) = default;

    ::test::docs::WithInternalEnum::InternalEnum e;
};

} // namespace docs
} // namespace test

namespace boost {
namespace serialization {

template <typename Archive>
void serialize(
    Archive& ar,
    ::test::docs::VeryPrivate& obj,
    const unsigned int /* version */)
{
    ar & boost::serialization::make_nvp("regularField", obj.regularField);
    ar & boost::serialization::make_nvp("oneMoreRegularField", obj.oneMoreRegularField);
}


template <typename Archive>
void serialize(
    Archive& ar,
    ::test::docs::VeryOpen& obj,
    const unsigned int /* version */)
{
    ar & boost::serialization::make_nvp("regularField", obj.regularField);
    ar & boost::serialization::make_nvp("hiddenSwitch", obj.hiddenSwitch);
}


template <typename Archive>
void serialize(
    Archive& ar,
    ::test::docs::MuchUnprotected& obj,
    const unsigned int /* version */)
{
    ar & boost::serialization::make_nvp("regularField", obj.regularField);
    ar & boost::serialization::make_nvp("oneMoreRegularField", obj.oneMoreRegularField);
    ar & boost::serialization::make_nvp("hiddenField", obj.hiddenField);
}


template <typename Archive>
void serialize(
    Archive& ar,
    ::test::docs::WithInternalEnum& obj,
    const unsigned int /* version */)
{
    ar & boost::serialization::make_nvp("e", obj.e);
}


} // namespace serialization
} // namespace boost

namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {

template <>
struct BindingTraits<::test::docs::VeryPrivate> : public BaseBindingTraits {
    static constexpr bool isStruct = true;
    static constexpr bool isBridged = true;
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/VeryPrivate";
    static constexpr const char* const objectiveCName = "YTDVeryPrivate";
    static constexpr const char* const cppName = "test::docs::VeryPrivate";
};

template <>
struct BindingTraits<::test::docs::SuchHidden> : public BaseBindingTraits {
    static constexpr bool isStruct = true;
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/SuchHidden";
    static constexpr const char* const objectiveCName = "YTDSuchHidden";
    static constexpr const char* const cppName = "test::docs::SuchHidden";
};

template <>
struct BindingTraits<::test::docs::SuchOptions> : public BaseBindingTraits {
    static constexpr bool isStruct = true;
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/SuchOptions";
    static constexpr const char* const objectiveCName = "YTDSuchOptions";
    static constexpr const char* const cppName = "test::docs::SuchOptions";
};

template <>
struct BindingTraits<::test::docs::TooInternal> : public BaseBindingTraits {
    using TopMostBaseType = ::test::docs::TooInternal;
    static constexpr bool isStrongInterface = true;
    static constexpr const char* const javaBindingUndecoratedName = "ru/test/docs/internal/TooInternalBinding";
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/TooInternal";
    static constexpr const char* const objectiveCName = "YTDTooInternal";
    static constexpr const char* const cppName = "test::docs::TooInternal";
};

template <>
struct BindingTraits<::test::docs::VeryOpen> : public BaseBindingTraits {
    static constexpr bool isStruct = true;
    static constexpr bool isBridged = true;
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/VeryOpen";
    static constexpr const char* const objectiveCName = "YTDVeryOpen";
    static constexpr const char* const cppName = "test::docs::VeryOpen";
};

template <>
struct BindingTraits<::test::docs::MuchUnprotected> : public BaseBindingTraits {
    static constexpr bool isStruct = true;
    static constexpr bool isBridged = true;
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/MuchUnprotected";
    static constexpr const char* const objectiveCName = "YTDMuchUnprotected";
    static constexpr const char* const cppName = "test::docs::MuchUnprotected";
};

template <>
struct BindingTraits<::test::docs::TooExternal> : public BaseBindingTraits {
    using TopMostBaseType = ::test::docs::TooExternal;
    static constexpr bool isStrongInterface = true;
    static constexpr const char* const javaBindingUndecoratedName = "ru/test/docs/internal/TooExternalBinding";
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/TooExternal";
    static constexpr const char* const objectiveCName = "YTDTooExternal";
    static constexpr const char* const cppName = "test::docs::TooExternal";
};

template <>
struct BindingTraits<::test::docs::WithInternalEnum> : public BaseBindingTraits {
    static constexpr bool isStruct = true;
    static constexpr bool isBridged = true;
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/WithInternalEnum";
    static constexpr const char* const objectiveCName = "YTDWithInternalEnum";
    static constexpr const char* const cppName = "test::docs::WithInternalEnum";
};

template <>
struct BindingTraits<::test::docs::WithInternalEnum::InternalEnum> : public BaseBindingTraits {
    static constexpr const char* const javaUndecoratedName = "ru/test/docs/WithInternalEnum$InternalEnum";
    static constexpr const char* const objectiveCName = "YTDWithInternalEnumInternalEnum";
    static constexpr const char* const cppName = "test::docs::WithInternalEnum::InternalEnum";
};

} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex
