#include "object.h"

#include <kernel/common_server/util/algorithm/container.h>
namespace NCS {
    namespace NHandlers {
        IItemPermissions::TFactory::TRegistrator<TTagsObjectPermissions> TTagsObjectPermissions::Registrator(TTagsObjectPermissions::GetTypeName());
    }
}
