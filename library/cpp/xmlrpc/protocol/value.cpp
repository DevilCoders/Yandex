#include "value.h"
#include "xml.h"
#include "types.h"

#include <util/generic/cast.h>
#include <util/string/cast.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/xml/encode/encodexml.h>
#include <util/generic/singleton.h>

namespace {
    using namespace NXmlRPC;

    template <class T>
    struct TValueBase: public IValue {
        typedef T TBase;

        inline TValueBase() {
        }

        inline TValueBase(const T& v)
            : V(v)
        {
        }

        bool IsA(const std::type_info& ti) const override {
            return ti == typeid(V);
        }

        void* Ptr() const override {
            return (void*)&V;
        }

        TStringBuf AsBlob() const override {
            ythrow TTypeConversionError() << "type mismatch";
        }

        bool AsBool() const override {
            ythrow TTypeConversionError() << "type mismatch";
        }

        TInstant AsDateTime() const override {
            ythrow TTypeConversionError() << "type mismatch";
        }

        double AsDouble() const override {
            ythrow TTypeConversionError() << "type mismatch";
        }

        i64 AsInteger() const override {
            ythrow TTypeConversionError() << "type mismatch";
        }

        TString AsString() const override {
            ythrow TTypeConversionError() << "type mismatch";
        }

        size_t ElementCount() const override {
            ythrow TTypeConversionError() << "type mismatch";
        }

        T V;
    };

    template <class T>
    struct TContainerValue: public TValueBase<T> {
        inline TContainerValue() {
        }

        inline TContainerValue(const T& v)
            : TValueBase<T>(v)
        {
        }

        size_t ElementCount() const override {
            return this->V.size();
        }
    };

    struct TStructValue: public TContainerValue<TStruct> {
        inline TStructValue(const TStruct& v)
            : TContainerValue<TStruct>(v)
        {
        }

        static inline IValue* ConstructFromNode(const NXml::TConstNode& t) {
            return new TStructValue(FirstChild(t));
        }

        inline TStructValue(NXml::TConstNode n) {
            while (!n.IsNull()) {
                Expect(n, TStringBuf("member"));

                V[n.Node("name").Value<TString>()] = Construct(n.Node("value"));
                n = NextSibling(n);
            }
        }

        void SerializeXml(IOutputStream& out) const override {
            out << TStringBuf("<struct>");

            for (const auto& it : V) {
                out << TStringBuf("<member>");

                //name
                {
                    out << TStringBuf("<name>");
                    out << EncodeXML(it.first.data());
                    out << TStringBuf("</name>");
                }

                //value
                it.second.SerializeXml(out);

                out << TStringBuf("</member>");
            }

            out << TStringBuf("</struct>");
        }
    };

    struct TArrayValue: public TContainerValue<TArray> {
        inline TArrayValue(const TArray& v)
            : TContainerValue<TArray>(v)
        {
        }

        static inline IValue* ConstructFromNode(const NXml::TConstNode& t) {
            return new TArrayValue(FirstChild(t));
        }

        inline TArrayValue(NXml::TConstNode n) {
            Expect(n, TStringBuf("data"));

            n = FirstChild(n);

            while (!n.IsNull()) {
                V.push_back(TValue(n));
                n = NextSibling(n);
            }
        }

        void SerializeXml(IOutputStream& out) const override {
            out << TStringBuf("<array><data>");

            for (const auto& it : V) {
                it.SerializeXml(out);
            }

            out << TStringBuf("</data></array>");
        }
    };

    template <class T>
    struct TNumericValue: public TValueBase<T> {
        inline TNumericValue(const T& v)
            : TValueBase<T>(v)
        {
        }

        bool AsBool() const override {
            return this->V;
        }

        double AsDouble() const override {
            return this->V;
        }

        i64 AsInteger() const override {
            return this->V;
        }

        TString AsString() const override {
            return ToString(this->V);
        }
    };

    struct TIntValue: public TNumericValue<i64> {
        inline TIntValue(i64 v)
            : TNumericValue<i64>(v)
        {
        }

        static inline IValue* ConstructFromNode(const NXml::TConstNode& t) {
            return new TIntValue(FromString<i64>(t.Value<TString>()));
        }

        void SerializeXml(IOutputStream& out) const override {
            out << TStringBuf("<int>");
            out << V;
            out << TStringBuf("</int>");
        }
    };

    struct TBoolValue: public TNumericValue<bool> {
        inline TBoolValue(bool v)
            : TNumericValue<bool>(v)
        {
        }

        static inline IValue* ConstructFromNode(const NXml::TConstNode& t) {
            return new TBoolValue(FromString<bool>(t.Value<TString>()));
        }

        void SerializeXml(IOutputStream& out) const override {
            out << TStringBuf("<boolean>");
            out << (unsigned int)V;
            out << TStringBuf("</boolean>");
        }
    };

    struct TDoubleValue: public TNumericValue<double> {
        inline TDoubleValue(double v)
            : TNumericValue<double>(v)
        {
        }

        static inline IValue* ConstructFromNode(const NXml::TConstNode& t) {
            return new TDoubleValue(FromString<double>(t.Value<TString>()));
        }

        void SerializeXml(IOutputStream& out) const override {
            out << TStringBuf("<double>");
            out << V;
            out << TStringBuf("</double>");
        }
    };

    struct TStringValue: public TValueBase<TString> {
        inline TStringValue(const TString& v)
            : TValueBase<TString>(v)
        {
        }

        static inline IValue* ConstructFromNode(const NXml::TConstNode& t) {
            return new TStringValue(t.Value<TString>());
        }

        TStringBuf AsBlob() const override {
            return V;
        }

        bool AsBool() const override {
            return FromString<bool>(V);
        }

        TInstant AsDateTime() const override {
            return TInstant::ParseIso8601Deprecated(V);
        }

        double AsDouble() const override {
            return FromString<double>(V);
        }

        i64 AsInteger() const override {
            return FromString<i64>(V);
        }

        TString AsString() const override {
            return V;
        }

        void SerializeXml(IOutputStream& out) const override {
            out << TStringBuf("<string>");
            out << EncodeXML(V.data());
            out << TStringBuf("</string>");
        }
    };

    struct TBinaryValue: public TValueBase<TString> {
        inline TBinaryValue(const TString& v)
            : TValueBase<TString>(v)
        {
        }

        static inline IValue* ConstructFromNode(const NXml::TConstNode& t) {
            return new TBinaryValue(Base64Decode(t.Value<TString>()));
        }

        TStringBuf AsBlob() const override {
            return V;
        }

        TString AsString() const override {
            return V;
        }

        void SerializeXml(IOutputStream& out) const override {
            out << TStringBuf("<base64>");
            out << Base64Encode(V);
            out << TStringBuf("</base64>");
        }
    };

    struct TDatetimeValue: public TValueBase<TInstant> {
        inline TDatetimeValue(const TInstant& v)
            : TValueBase<TInstant>(v)
        {
        }

        static inline IValue* ConstructFromNode(const NXml::TConstNode& t) {
            return new TDatetimeValue(TInstant::ParseIso8601Deprecated(t.Value<TString>()));
        }

        TInstant AsDateTime() const override {
            return V;
        }

        TString AsString() const override {
            char buf[DATE_8601_LEN];

            sprint_date8601(buf, V.Seconds());

            return buf;
        }

        void SerializeXml(IOutputStream& out) const override {
            out << TStringBuf("<datetime.iso8601>");
            out << AsString();
            out << TStringBuf("</datetime.iso8601>");
        }
    };

    struct TDatetimeWithoutTimezoneValue: public TValueBase<TInstantWithoutTimezone> {
        inline TDatetimeWithoutTimezoneValue(const TInstantWithoutTimezone& v)
            : TValueBase<TInstantWithoutTimezone>(v)
        {
        }

        static inline IValue* ConstructFromNode(const NXml::TConstNode& t) {
            return new TDatetimeWithoutTimezoneValue(TInstantWithoutTimezone(TInstant::ParseIso8601(t.Value<TString>())));
        }

        TInstant AsDateTime() const override {
            return V;
        }

        TString AsString() const override {
            return V.FormatGmTime("%Y%m%dT%H:%M:%S");
        }

        void SerializeXml(IOutputStream& out) const override {
            out << TStringBuf("<dateTime.iso8601>");
            out << AsString();
            out << TStringBuf("</dateTime.iso8601>");
        }
    };

    struct TNull {
    };

    struct TNullValue: public TValueBase<TNull> {
        inline TNullValue() noexcept {
            Ref();
        }

        void SerializeXml(IOutputStream& out) const override {
            out << TStringBuf("<nil/>");
        }
    };

    typedef IValue* (*TValueConstructorFunc)(const NXml::TConstNode&);

    struct TValueConstructor: public THashMap<TStringBuf, TValueConstructorFunc> {
        inline TValueConstructor() {
            Add(TStringBuf("struct"), TStructValue::ConstructFromNode);
            Add(TStringBuf("array"), TArrayValue::ConstructFromNode);
            Add(TStringBuf("int"), TIntValue::ConstructFromNode);
            Add(TStringBuf("i4"), TIntValue::ConstructFromNode);
            Add(TStringBuf("double"), TDoubleValue::ConstructFromNode);
            Add(TStringBuf("boolean"), TBoolValue::ConstructFromNode);
            Add(TStringBuf("string"), TStringValue::ConstructFromNode);
            Add(TStringBuf("base64"), TBinaryValue::ConstructFromNode);
            Add(TStringBuf("datetime.iso8601"), TDatetimeValue::ConstructFromNode);
            Add(TStringBuf("dateTime.iso8601"), TDatetimeWithoutTimezoneValue::ConstructFromNode);
        }

        inline void Add(const TStringBuf& key, TValueConstructorFunc func) {
            (*this)[key] = func;
        }

        inline IValue* Construct(const TStringBuf& key, const NXml::TConstNode& t) const {
            const_iterator it = find(key);

            if (it != end()) {
                return it->second(t);
            }

            ythrow TXmlRPCError() << "unsupported type " << key;
        }

        static inline const TValueConstructor& Instance() {
            return *Singleton<TValueConstructor>();
        }
    };
}

namespace NXmlRPC {
    IValue* ConstructXmlRPCValue(const NXml::TConstNode& t) {
        const TString name = t.Name();

        if (name == TStringBuf("value")) {
            return ConstructXmlRPCValue(FirstChild(t));
        }

        if (name == TStringBuf("nil")) {
            return Null();
        }

        return TValueConstructor::Instance().Construct(name, t);
    }

#define DECLARE2(T1, T2)                        \
    IValue* ConstructXmlRPCValue(const T2& v) { \
        return new T1(T1::TBase(v));            \
    }
#define DECLARE(T1) DECLARE2(T1, T1::TBase)

    DECLARE(TStringValue)
    DECLARE(TBoolValue)
    DECLARE(TStructValue)
    DECLARE(TArrayValue)
    DECLARE(TDoubleValue)
    DECLARE(TDatetimeValue)
    DECLARE(TDatetimeWithoutTimezoneValue)

    //some magick
    DECLARE2(TStringValue, TStringBuf)

    DECLARE2(TDoubleValue, float)
    DECLARE2(TDoubleValue, long double)

    DECLARE2(TIntValue, signed short)
    DECLARE2(TIntValue, unsigned short)
    DECLARE2(TIntValue, signed int)
    DECLARE2(TIntValue, unsigned int)
    DECLARE2(TIntValue, signed long)
    DECLARE2(TIntValue, unsigned long)
    DECLARE2(TIntValue, signed long long)
    DECLARE2(TIntValue, unsigned long long)

#undef DECLARE
#undef DECLARE2

    IValue* Null() noexcept {
        return Singleton<TNullValue>();
    }

#define NOP(X, T) X
#define INTCVT(X, T) SafeIntegerCast<T>(X)
#define DEFCASTX(T, M, H)          \
    template <>                    \
    T DoCast<T>(const IValue* v) { \
        return H(v->M(), T);       \
    }
#define DEFCAST(T, M) DEFCASTX(T, M, NOP)
#define DEFINTCAST(T) DEFCASTX(T, AsInteger, INTCVT)

    DEFCAST(TString, AsString)
    DEFCAST(TStringBuf, AsBlob)
    DEFCAST(double, AsDouble)
    DEFCAST(long double, AsDouble)
    DEFCAST(float, AsDouble)
    DEFCAST(TInstant, AsDateTime)
    DEFCAST(bool, AsBool)

    DEFINTCAST(signed short)
    DEFINTCAST(unsigned short)
    DEFINTCAST(signed int)
    DEFINTCAST(unsigned int)
    DEFINTCAST(signed long)
    DEFINTCAST(unsigned long)
    DEFINTCAST(signed long long)
    DEFINTCAST(unsigned long long)

#undef NOP
#undef INTCVT
#undef DEFCAST
#undef DEFCASTX
#undef DEFINTCAST

    TValue ParseValue(IInputStream& in) {
        return ParseValue(in.ReadAll());
    }

    TValue ParseValue(const TString& data) {
        NXml::TDocument doc(data, NXml::TDocument::String);

        return TValue(NXml::TConstNode(doc.Root()));
    }
}

IValue::IValue() {
}

IValue::~IValue() {
}

void TValue::SerializeXml(IOutputStream& out) const {
    out << TStringBuf("<value>");
    V_->SerializeXml(out);
    out << TStringBuf("</value>");
}

const TValue& TValue::operator[](size_t n) const {
    return Array().at(n);
}

TValue& TValue::operator[](size_t n) {
    return Array().at(n);
}

const TValue& TValue::operator[](const TStringBuf& key) const {
    return Struct().Find(key);
}

TValue& TValue::operator[](const TStringBuf& key) {
    return Struct()[key];
}

const TValue& TStruct::Find(const TStringBuf& key) const {
    const_iterator it = find(key);

    if (it == end()) {
        ythrow TXmlRPCError() << "no such key " << key << "in struct";
    }

    return it->second;
}

const TValue& TArray::IndexOrNull(size_t n) const noexcept {
    if (n < size()) {
        return (*this)[n];
    }

    return *Singleton<TValue>();
}

template <>
void Out<TValue>(IOutputStream& o, const TValue& v) {
    v.SerializeXml(o);
}
