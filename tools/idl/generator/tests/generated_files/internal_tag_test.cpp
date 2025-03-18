#include <test/docs/internal_tag_test.h>

#include <yandex/maps/runtime/any/guid_registration.h>
#include <yandex/maps/runtime/bindings/internal/archive_generator.h>
#include <yandex/maps/runtime/bindings/internal/archive_reader.h>
#include <yandex/maps/runtime/bindings/internal/archive_writer.h>
#include <yandex/maps/runtime/serialization/ptr.h>

#include <boost/optional.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/optional.hpp>

#include <memory>

namespace test {
namespace docs {

VeryPrivate::VeryPrivate()
    : regularField(),
      oneMoreRegularField()
{
}

VeryPrivate::VeryPrivate(
    int regularField,
    float oneMoreRegularField)
    : regularField(regularField),
      oneMoreRegularField(oneMoreRegularField)
{
}

SuchHidden::SuchHidden(
    float regularField,
    int oneMoreRegularField,
    float twoMoreRegularFields)
    : regularField_(regularField),
      oneMoreRegularField_(oneMoreRegularField),
      twoMoreRegularFields_(twoMoreRegularFields)
{
}



SuchOptions::SuchOptions(
    const std::unique_ptr<::test::docs::TooInternal>& interfaceField)
    : interfaceField_(interfaceField)
{
}



VeryOpen::VeryOpen()
    : regularField(),
      hiddenSwitch()
{
}

VeryOpen::VeryOpen(
    int regularField,
    const boost::optional<bool>& hiddenSwitch)
    : regularField(regularField),
      hiddenSwitch(hiddenSwitch)
{
}

MuchUnprotected::MuchUnprotected()
    : regularField(),
      oneMoreRegularField(),
      hiddenField()
{
}

MuchUnprotected::MuchUnprotected(
    float regularField,
    bool oneMoreRegularField,
    const boost::optional<int>& hiddenField)
    : regularField(regularField),
      oneMoreRegularField(oneMoreRegularField),
      hiddenField(hiddenField)
{
}

WithInternalEnum::WithInternalEnum()
    : e()
{
}

WithInternalEnum::WithInternalEnum(
    ::test::docs::WithInternalEnum::InternalEnum e)
    : e(e)
{
}

} // namespace docs
} // namespace test

REGISTER_GUID(test::docs::VeryPrivate);


REGISTER_GUID(test::docs::VeryOpen);


REGISTER_GUID(test::docs::MuchUnprotected);


REGISTER_GUID(test::docs::WithInternalEnum);

