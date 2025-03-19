#include "snapshot_object.h"

namespace NCS {
    namespace NSnapshots {
        class TMappedObjectUpsertHandler: public TObjectUpsertHandler<TMappedObject, IBaseServer> {
            using TBase = TObjectUpsertHandler<TMappedObject, IBaseServer>;
            TRegistrationHandlerDefaultConfig ConfigRegistrator = { "dummy" };
        public:
            using TBase::TBase;
        };

        class TMappedObjectRemoveHandler: public TObjectRemoveHandler<TMappedObject, IBaseServer> {
            using TBase = TObjectRemoveHandler<TMappedObject, IBaseServer>;
            TRegistrationHandlerDefaultConfig ConfigRegistrator = { "dummy" };
        public:
            using TBase::TBase;
        };

        class TMappedObjectInfoHandler: public TObjectInfoHandler<TMappedObject, IBaseServer> {
            using TBase = TObjectInfoHandler<TMappedObject, IBaseServer>;
            TRegistrationHandlerDefaultConfig ConfigRegistrator = { "dummy" };
        public:
            using TBase::TBase;
        };
    }
}
