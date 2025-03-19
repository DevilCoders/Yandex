#pragma once

#include <library/cpp/logger/global/global.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/generic/typetraits.h>
#include <util/memory/blob.h>
#include <util/system/type_name.h>

namespace NCommonProxy {

    class TObject : public TAtomicRefCount<TObject> {
    public:
        typedef TIntrusivePtr<TObject> TPtr;

    public:
        virtual ~TObject() {}
    };

    template<class T>
    class TObjectHolder : public TObject {
    public:
        template<class... Args>
        inline TObjectHolder(Args&&... args)
            : Ptr(MakeAtomicShared<T>(std::forward<Args>(args)...))
        {}

        inline TObjectHolder(T* ptr)
            : Ptr(ptr)
        {}

        inline T* Get() const noexcept {
            return Ptr.Get();
        }

        inline T* operator->() const noexcept {
            return Get();
        }

        inline T& operator*() const noexcept {
            return *Ptr;
        }

    private:
        TAtomicSharedPtr<T> Ptr;
    };

    class TMetaData {
    public:
        enum TDataType {
            dtUNKNOWN,
            dtBLOB,
            dtBOOL,
            dtDOUBLE,
            dtINT,
            dtSTRING,
            dtOBJECT
        };

        class IObjectChecker: public TAtomicRefCount<IObjectChecker> {
        public:
            virtual ~IObjectChecker() {}
            virtual bool Correct(const TObject* object) const = 0;
            virtual TString Type() const = 0;
        };

        template <class T>
        class TObjectChecker : public IObjectChecker {
        public:
            virtual bool Correct(const TObject* object) const override {
                return !object || dynamic_cast<const T*>(object);
            }
            virtual TString Type() const override {
                return typeid(T).name();
            }
        };

        struct TValueMeta {
            TValueMeta(TDataType type)
                : Type(type)
            {
                if (type == dtOBJECT) {
                    ObjectChecker = MakeIntrusive<TObjectChecker<TObject>>();
                }
            }

            TValueMeta(TIntrusivePtr<IObjectChecker> objChecker)
                : Type(dtOBJECT)
                , ObjectChecker(objChecker)
            {}

            TString ToString() const;

            TDataType Type;
            TIntrusivePtr<IObjectChecker> ObjectChecker;
        };

        typedef THashMap<TString, TValueMeta> TFields;

        template<class T>
        class TObjectPtr : public TObject {
        public:
            inline TObjectPtr(T* ptr)
                : Ptr(ptr)
            {}

            inline T* Get() const noexcept {
                return Ptr;
            }

        private:
            T* Ptr;
        };

    public:
        TMetaData();
        bool IsSubsetOf(const TMetaData& other) const;
        bool TypeIs(const TString& name, TDataType type) const;
        void Register(const TString& name, const TValueMeta& meta);

        template<class T>
        void RegisterObject(const TString& name) {
            Register(name, TValueMeta(MakeIntrusive<TObjectChecker<T>>()));
        }

        template<class T>
        void RegisterPointer(const TString& name) {
            RegisterObject<TObjectPtr<T>>(name);
        }

        TString ToString() const;
        const TFields& GetFields() const {
            return Fields;
        }

        static const TMetaData Empty;

    private:
        TFields Fields;
    };

    class TDataSet : public TAtomicRefCount<TDataSet> {
    public:
        typedef TIntrusivePtr<TDataSet> TPtr;

    public:
        TDataSet(const TMetaData& metaData);

        static TPtr Construct(const TMetaData& metaData) {
            return MakeIntrusive<TDataSet>(metaData);
        }

        const TMetaData& GetMetaData() const {
            return MetaData;
        }

        template<class T>
        typename TTypeTraits<T>::TFuncParam Get(const TString& name) const;

        template<class T>
        inline void Set(const TString& name, typename TTypeTraits<T>::TFuncParam value) {
            SetImpl<T>(name, value);
            SetFields.insert(name);
        }

        template<class T>
        inline void SetObject(const TString& name, TIntrusivePtr<T> object) {
            auto field = MetaData.GetFields().find(name);
            if (field == MetaData.GetFields().end() || field->second.Type != TMetaData::dtOBJECT)
                ythrow yexception() << "Cannot set " << name << ": it must be " << ToString(field->second.Type);

            CHECK_WITH_LOG(field->second.ObjectChecker);

            if (!field->second.ObjectChecker->Correct(object.Get()))
                ythrow yexception() << "Cannot set " << name << ": it is not " << field->second.ObjectChecker->Type();
            Set<TObject::TPtr>(name, object);
        }

        template<class T>
        inline TIntrusivePtr<T> GetObject(const TString& name) const {
            TObject::TPtr obj = GetObjectPtr(name);
            if (!obj)
                return nullptr;
            TIntrusivePtr<T> result = dynamic_cast<T*>(obj.Get());
            if (!result)
                ythrow yexception() << "Cannot get " << name << ": it is not " << TypeName<T>();
            return result;
        }

        template <class T>
        inline void SetPointer(const TString& name, T* pointer) {
            SetObject<TMetaData::TObjectPtr<T>>(name, MakeIntrusive<TMetaData::TObjectPtr<T>>(pointer));
        }

        template <class T>
        inline T* GetPointer(const TString& name) const {
            return GetObject<TMetaData::TObjectPtr<T>>(name)->Get();
        }

        bool AllFieldsSet() const;

        bool IsSet(const TString& field) const {
            return SetFields.contains(field);
        }

    private:
        template<class T>
        void SetImpl(const TString& name, typename TTypeTraits<T>::TFuncParam value);
        TObject::TPtr GetObjectPtr(const TString& name) const;

        const TMetaData& MetaData;
        THashMap<TString, i64> Ints;
        THashMap<TString, bool> Bools;
        THashMap<TString, double> Doubles;
        THashMap<TString, TString> Strings;
        THashMap<TString, TBlob> Blobs;
        THashSet<TString> SetFields;
        THashMap<TString, TObject::TPtr> Objects;
    };
}
