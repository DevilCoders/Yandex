#include "fake.h"

namespace NExternalAPI {

    TFakeRequestCustomization::TFactory::TRegistrator<TFakeRequestCustomization> TFakeRequestCustomization::Registrator(TFakeRequestCustomization::GetTypeName());

}
