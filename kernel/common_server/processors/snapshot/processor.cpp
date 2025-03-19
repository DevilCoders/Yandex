#include "processor.h"
#include "snapshot_object.h"

namespace NCS {
    namespace NSnapshots {
        class TMappedObjectUpsertHandler: public TObjectUpsertHandler<TMappedObject> {
            using TBase = TObjectUpsertHandler<TMappedObject>;
            TRegistrationHandlerDefaultConfig ConfigRegistrator = { "dummy" };
        public:
            using TBase::TBase;
        };

        class TMappedObjectRemoveHandler: public TObjectRemoveHandler<TMappedObject> {
            using TBase = TObjectRemoveHandler<TMappedObject>;
            TRegistrationHandlerDefaultConfig ConfigRegistrator = { "dummy" };
        public:
            using TBase::TBase;
        };

        class TMappedObjectInfoHandler: public TObjectInfoHandler<TMappedObject> {
            using TBase = TObjectInfoHandler<TMappedObject>;
            TRegistrationHandlerDefaultConfig ConfigRegistrator = { "dummy" };
        public:
            using TBase::TBase;
        };
    }
}
