#pragma once

#include <kernel/search_types/search_types.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/system/defaults.h>

namespace NGroupingAttrs {

class TConfig {
public:
    enum Type {
        I16,
        I32,
        I64,
        BAD
    };

    enum Mode {
        Index,
        Search
    };

    typedef ui8 TPackedType;

    void InitNonRealTime(); // Sorts attributes
    static const size_t AttrConfigReserv = 512;

    TConfig(Mode usage = Search);
    TConfig(const TConfig&) = default;

    /* Init from TDocsAttrs */
    //unique - array with of bool with length tlen
    void Init(const TVector<TString>& names, const TPackedType* types, size_t tlen, const bool* unique = nullptr);
    /* Init from TCreator */
    /* WithTypes means lines like ([:AttrName:](:[0-2])?)+ */
    void InitFromStringWithTypes(const char* str);
    void InitFromStringWithTypes(const TString& str);

    /* WithBytes means lines like ([:AttrName:]\.d2c(:[1-8])\n)+ */
    void InitFromStringWithBytes(const char* str);
    void InitFromStringWithBytes(const TString& str);
    void InitFromFileWithBytes(const char* filename);

    void MergeConfig(const TConfig& other);

    /* WithTypes */
    TString ToString() const;

    /* Only for the Yandex.Server indexer*/
    Mode UsageMode() const {
        return Usage;
    }

    static const char* IndexAuxAttrName();

    /* Indicates weather the attribute has a corresponding <attrname>.c2n file. */
    bool IsAttrNamed(ui32 attrnum) const;

    bool IsAttrUnique(size_t attrnum) const;

    /* NB! Invalidates previous attrnums */
    int AddAttr(const TString& attrname, Type type = I32, bool isRealTime = false, bool isUnique = false); /* default is _not_ max possible */

    bool HasAttr(TStringBuf attrname) const;

    ui32 AttrNum(TStringBuf attrname) const;

    const char* AttrName(ui32 attrnum) const;

    ui32 AttrCount() const;

    Type AttrType(ui32 attrnum) const;

    TCateg AttrMaxValue(ui32 attrnum) const;

    bool operator!=(const TConfig& other) const;
    bool operator==(const TConfig& other) const;
    TConfig& operator=(const TConfig& other);

    static Type GetType(TCateg value);

public:
    static constexpr ui32 NotFound = Max<ui32>();

private:
    void InitUsage();
    void InitHash();
    void InitFromStreamWithBytes(IInputStream& input);

private:
    struct TAttrData {
        TString Name;
        Type Size;
        bool IsNamed; /* Only for the Yandex.Server indexer */
        bool IsUnique;

        TAttrData();

        TAttrData(TString name, Type type, bool isNamed, bool isUnique);

        bool operator==(const TAttrData& other) const;
        bool operator!=(const TAttrData& other) const;
        bool operator>(const TAttrData& other) const;
    };

    typedef TVector<TAttrData> TAttrDatas;

    TAttrDatas Data;
    THashMap<TString, ui32> DataHash;
    Mode Usage;
};

}
