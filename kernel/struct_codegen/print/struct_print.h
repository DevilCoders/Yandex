#pragma once

// Methods for working with erf records with dynamically known structure.
// Copyright (c) 2011 Yandex, LLC. All rights reserved.

#include <library/cpp/packedtypes/packedfloat.h>
#include <util/generic/noncopyable.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <kernel/struct_codegen/reflection/reflection.h>


#define DO_VISIT(type) \
void Visit(const char* name, const type& value) { \
    Collector.Collect(name, value); \
}

struct TMangoWeightyWordWeights; // needs for mango. what do i need to do with it ?!!
struct TMangoDiversitySerializeData; // needs for mango

template <typename IVisitor, typename TCollector>
class TFieldsVisitor: public IVisitor, public TNonCopyable {
private:
    TCollector& Collector;

public:
    explicit TFieldsVisitor(TCollector& collector)
        : Collector(collector)
    {
    }
    DO_VISIT(ui32)
    DO_VISIT(float)
    DO_VISIT(f16)
    DO_VISIT(ui16)
    DO_VISIT(ui8)
    DO_VISIT(TMangoWeightyWordWeights)
    DO_VISIT(TMangoDiversitySerializeData)
};

#undef DO_VISIT

template <typename TStruct, typename TCollector>
void VisitStructHeader(const typename TStruct::TFieldMask& mask, TCollector& collector) {
    TFieldsVisitor<typename TStruct::IFieldsVisitor, TCollector> visitor(collector);
    TStruct::VisitFields(visitor, mask);
}

template <typename TStruct, typename TCollector>
void VisitStructFields(const TStruct& data, TCollector& collector) {
    TFieldsVisitor<typename TStruct::IFieldsVisitor, TCollector> visitor(collector);
    NErf::VisitFields(data, visitor);
}

template <typename TStruct, typename TCollector>
void VisitStructFields(const TStruct& data, const typename TStruct::TFieldMask& mask, TCollector& collector) {
    TFieldsVisitor<typename TStruct::IFieldsVisitor, TCollector> visitor(collector);
    NErf::VisitFields(data, visitor, mask);
}

/////////////////////////////////////////////////////////////////////////////////////////

class TDelimitedStrFieldsCollector : public TNonCopyable {
private:
    IOutputStream& Out;
    const char* Prefix;
    bool IsFirst;
    char Delimeter;

public:
    explicit TDelimitedStrFieldsCollector(IOutputStream& out, const char delimeter, const char* prefix = nullptr)
        : Out(out)
        , Prefix(prefix)
        , IsFirst(true)
        , Delimeter(delimeter)
    {
    }
    template <typename T>
    void Collect(const T& value) {
        PreCollect();
        Out << value;
    }

    template <typename TName, typename TValue>
    void Collect(const TName& name, const TValue& value) {
        PreCollect();
        Out << name << ": " << value;
    }

private:
    void PreCollect() {
        if (!IsFirst)
            Out << Delimeter;
        IsFirst = false;
        if (Prefix)
            Out << Prefix;
    }

};

class TTabDelimitedStrNamesCollector : public TNonCopyable {
private:
    TDelimitedStrFieldsCollector Collector;

public:
    TTabDelimitedStrNamesCollector(IOutputStream& out, const char* prefix)
        : Collector(out, '\t', prefix)
    {
    }

    template <typename T>
    void Collect(const char* name, const T& value) {
        Collector.Collect(name);
        Y_UNUSED(value);
    }
};

class TTabDelimitedStrValuesCollector : public TNonCopyable {
private:
    TDelimitedStrFieldsCollector Collector;

public:
    explicit TTabDelimitedStrValuesCollector(IOutputStream& out)
        : Collector(out, '\t')
    {
    }

    template <typename T>
    void Collect(const char* name, const T& value) {
        Y_UNUSED(name);
        Collector.Collect(value);
    }
};

class TNewLineDelimitedStrPairsCollector : public TNonCopyable {
private:
    TDelimitedStrFieldsCollector Collector;

public:
    explicit TNewLineDelimitedStrPairsCollector(IOutputStream& out)
        : Collector(out, '\n', " ")
    {
    }

    template <typename T>
    void Collect(const char* name, const T& value) {
        Collector.Collect(name, value);
    }
};

template <typename TStruct>
class TTabDelimitedStrStructPrinter {
public:
    static TString GetHeader(const typename TStruct::TFieldMask& mask, const char* prefix = nullptr) {
        TStringStream stringStream;
        TTabDelimitedStrNamesCollector collector(stringStream, prefix);
        VisitStructHeader<TStruct>(mask, collector);
        return stringStream.Str();
    }

    static TString PrintFields(const TStruct& data, const typename TStruct::TFieldMask& mask) {
        TStringStream stringStream;
        TTabDelimitedStrValuesCollector collector(stringStream);
        VisitStructFields(data, mask, collector);
        return stringStream.Str();
    }
};

template <typename TStruct>
class TCuteStrStructPrinter {
public:
    static TString GetHeader(const typename TStruct::TFieldMask& mask, const char* prefix = nullptr) {
        TStringStream stringStream;
        TTabDelimitedStrNamesCollector collector(stringStream, prefix);
        VisitStructHeader<TStruct>(mask, collector);
        return stringStream.Str();
    }

    static TString PrintFields(const TStruct& data, const typename TStruct::TFieldMask& mask) {
        TStringStream stringStream;
        TNewLineDelimitedStrPairsCollector collector(stringStream);
        VisitStructFields(data, mask, collector);
        return stringStream.Str();
    }
};
