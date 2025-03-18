#include <test/docs/test.h>

#include <yandex/maps/runtime/any/guid_registration.h>
#include <yandex/maps/runtime/bindings/internal/archive_generator.h>
#include <yandex/maps/runtime/bindings/internal/archive_reader.h>
#include <yandex/maps/runtime/bindings/internal/archive_writer.h>
#include <yandex/maps/runtime/serialization/chrono.h>
#include <yandex/maps/runtime/serialization/ptr.h>
#include <yandex/maps/runtime/time.h>

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/string.hpp>

#include <string>

namespace test {
namespace docs {

Struct::Struct()
    : i()
{
}

Struct::Struct(
    int i)
    : i(i)
{
}

Response::Response()
    : i(),
      f()
{
}

Response::Response(
    int i,
    float f)
    : i(i),
      f(f)
{
}

OptionsStructure::OptionsStructure(
    const std::string& empty,
    const std::string& filled)
    : empty_(empty),
      filled_(filled)
{
}



DefaultValue::DefaultValue()
    : filled()
{
}

DefaultValue::DefaultValue(
    const std::string& filled)
    : filled(filled)
{
}

CombinedValues::CombinedValues()
    : empty(),
      filled()
{
}

CombinedValues::CombinedValues(
    const std::string& empty,
    const std::string& filled)
    : empty(empty),
      filled(filled)
{
}

DefaultTimeintervalValue::DefaultTimeintervalValue()
    : empty(),
      filled()
{
}

DefaultTimeintervalValue::DefaultTimeintervalValue(
    ::yandex::maps::runtime::TimeInterval empty,
    ::yandex::maps::runtime::TimeInterval filled)
    : empty(empty),
      filled(filled)
{
}

} // namespace docs
} // namespace test

REGISTER_GUID(test::docs::Response);

