#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <kernel/groupattrs/config.h>

class IOutputStream;
class IInputStream;

class TFullDocAttrs {
public:
    enum EAttrType {
        // search attributes for key/inv and query language
        AttrSearchLiteral = 0x0001, // literal-type
        AttrSearchUrl     = 0x0002, // url-type
        AttrSearchDate    = 0x0004, // date-type
        AttrSearchInteger = 0x0008, // int-type
        // grouping attributes for aof/atr and form fields */
        AttrGrName        = 0x0010, // given as 'n' in .c2n map, one needs to convert
        AttrGrInt         = 0x0020, // given as 'c' in .c2n map
        // archive attributes (properties)
        AttrArcText       = 0x0040, // for dir/arc and search report
        AttrArcFull       = 0x0080, // for tdr/tag
        // auxiliary document properties for the tuning of indexator
        AttrAuxPars       = 0x0100, // indexer will set it as a parser property before processing the document
        //erf attributes
        AttrErf           = 0x0200,
        // archive attributes (binary properties)
        AttrArcBin        = 0x0400, // binary description of document in arc
        //zone's position
        AttrSearchZones   = 0x01000,
        AttrArcZones      = 0x02000,
        AttrArcAttrs      = 0x04000,
        AttrSearchLitPos  = 0x08000,
        AttrSearchIntPos  = 0x10000,
        AttrSearchLemma   = 0x20000,
    };
public:
    struct TAttr {
        TString Name;
        TString Value;
        ui32 Type; // a mask built from EAttrType
        NGroupingAttrs::TConfig::Type SizeOfInt;

        TAttr()
            : Type(0)
            , SizeOfInt(NGroupingAttrs::TConfig::I32)
        {
        }

        TAttr(const TString& name, const TString& value, ui32 type)
            : Name(name)
            , Value(value)
            , Type(type)
            , SizeOfInt(NGroupingAttrs::TConfig::I32)
        {
        }

        TAttr(const TString& name, const TString& value, ui32 type, NGroupingAttrs::TConfig::Type sizeOfInt)
            : Name(name)
            , Value(value)
            , Type(type)
            , SizeOfInt(sizeOfInt)
        {
        }

        //! loads attributes saved by PackTo(out, mask, packType = true)
        void Load(IInputStream* in);
    };
private:
    typedef TVector<TAttr> TAttrs;
    TAttrs Attrs;
public:
    typedef TAttrs::const_iterator TConstIterator;
    TConstIterator Begin() const {
        return Attrs.begin();
    }
    TConstIterator End() const {
        return Attrs.end();
    }
    void AddAttr(const TString& attrName, const TString& attrValue, ui32 type) {
        Attrs.push_back(TAttr(attrName, attrValue, type));
    }
    void AddAttr(const TString& attrName, const TString& attrValue, ui32 type, NGroupingAttrs::TConfig::Type sizeOfInt) {
        Attrs.push_back(TAttr(attrName, attrValue, type, sizeOfInt));
    }
    void Append(const TFullDocAttrs& other) {
        Attrs.insert(Attrs.end(), other.Attrs.begin(), other.Attrs.end());
    }
    void PackTo(IOutputStream& out, ui32 mask, bool packAttrType = false) const;
    void UnpackFrom(IInputStream& in);
};

void UnpackFrom(IInputStream* in, THashMap<TString, TString>* out);

inline void UnpackFrom(IInputStream& in, TFullDocAttrs& out) {
   out.UnpackFrom(in);
}

