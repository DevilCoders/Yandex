#pragma once

#include <util/generic/array_ref.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>

class IInputStream;
class IOutputStream;

namespace NOffroad {
    /**
     * Base class for offroad models.
     *
     * Used for type checking when loading model from stream using standard ::Load or ::Save methods
     */
    class ISerializableModel {
    public:
        ISerializableModel(const TString& typeName)
            : TypeName_(typeName)
        {
        }
        virtual ~ISerializableModel() {
        }

        void Load(IInputStream* in);
        void Save(IOutputStream* out) const;

        void Load(const TBlob& blob);
        void Load(const TArrayRef<const char>& region);
        void Load(const TString& path);
        void Save(const TString& path) const;

        const TString& TypeName() const;

        static TString TypeName(IInputStream* in);
        static TString TypeName(const TString& path);

    protected:
        // TODO: rename SaveData, LoadDate & move out. SaveLoad code does not belong to models, models shouldn't have virtual functions.

        virtual void DoLoad(IInputStream* in) = 0;
        virtual void DoSave(IOutputStream* out) const = 0;

        void ThrowInvalidModelType(const TString& typeName);

    private:
        TString TypeName_;
    };

    template <class Model>
    class TSerializableModel: public ISerializableModel {
    public:
        TSerializableModel()
            : ISerializableModel(Model::TypeName())
        {
        }
    };

}
