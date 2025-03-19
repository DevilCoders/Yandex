#include "fake.h"
namespace NCS {
    namespace NObfuscator {
        TFakeObfuscator::TFactory::TRegistrator<TFakeObfuscator> TFakeObfuscator::Registrator(TFakeObfuscator::GetTypeName());
    }
}
