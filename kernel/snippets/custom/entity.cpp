#include "entity.h"

#include <kernel/snippets/sent_match/extra_attrs.h>

#include <kernel/snippets/entityclassify/entitycl.h>

namespace NSnippets {
    void TEntityDataReplacer::DoWork(TReplaceManager* manager) {
        manager->GetExtraSnipAttrs().SetEntityClassifyResult(EntityClassify(manager->GetContext().Cfg, EntityView));
    }
}
