#include "data_accessor.h"
#include <library/cpp/logger/global/global.h>

namespace NRTYArchive {
    IDataAccessor::TPtr IDataAccessor::Create(const TFsPath& path, const TConstructContext& context) {
        IDataAccessor::TPtr result = TFactory::Construct(context.Type, path, context, true);
        VERIFY_WITH_LOG(!!result, "No exists implementation read accessor for type");
        return result;
    }

    IWritableDataAccessor::TPtr IWritableDataAccessor::Create(const TFsPath& path, const TConstructContext& context) {
        IWritableDataAccessor::TPtr result = TFactory::Construct(context.Type, path, context, false);
        VERIFY_WITH_LOG(!!result, "No exists implementation write accessor for type");
        return result;
    }
}
