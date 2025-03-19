#pragma once

#include <kernel/doom/standard_models/standard_models.h>

#include <library/cpp/resource/resource.h>

#include <util/memory/blob.h>
#include <util/string/cast.h>

namespace NDoom {

struct TStandardIoModelsStorage {
    template <class TModel>
    static TModel Model(EStandardIoModel model) {
        Y_VERIFY(model != NoStandardIoModel);
        TBlob serializedModel = TBlob::FromString(NResource::Find("standard_io_model_" + ToString<EStandardIoModel>(model)));
        TModel result;
        result.Load(serializedModel);
        return result;
    }
};


} // namespace NDoom
