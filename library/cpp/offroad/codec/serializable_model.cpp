#include "serializable_model.h"

#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/stream/mem.h>
#include <util/ysaveload.h>

namespace NOffroad {
    const TString& ISerializableModel::TypeName() const {
        return TypeName_;
    }

    void ISerializableModel::Load(IInputStream* in) {
        TString typeName = TypeName(in);
        if (typeName != TypeName_)
            ThrowInvalidModelType(typeName);
        DoLoad(in);
    }

    void ISerializableModel::Load(const TBlob& blob) {
        TMemoryInput input(blob.AsCharPtr(), blob.Size());
        Load(&input);
    }

    void ISerializableModel::Load(const TArrayRef<const char>& region) {
        TMemoryInput input(region.data(), region.size());
        Load(&input);
    }

    void ISerializableModel::Load(const TString& path) {
        TIFStream input(path);
        Load(&input);
    }

    void ISerializableModel::Save(IOutputStream* out) const {
        ::Save(out, TypeName_);
        DoSave(out);
    }

    void ISerializableModel::Save(const TString& path) const {
        TOFStream output(path);
        Save(&output);
    }

    TString ISerializableModel::TypeName(IInputStream* in) {
        TString result;
        ::Load(in, result);
        return result;
    }

    TString ISerializableModel::TypeName(const TString& path) {
        TIFStream input(path);
        return TypeName(&input);
    }

    void ISerializableModel::ThrowInvalidModelType(const TString& typeName) {
        ythrow yexception() << "Trying to read model of type \"" << typeName << "\""
                            << " into the model of type \"" << TypeName_ << "\"";
    }

}
