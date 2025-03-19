#pragma once

#include "markup.h"
#include <kernel/fio/fio.h>
#include <library/cpp/langmask/langmask.h>
#include <util/system/yassert.h>
#include <util/string/vector.h>

class TFioMarkup: public NSearchQuery::TMarkupData<NSearchQuery::MT_FIO> {
public:
    struct TField {
        NSearchQuery::TRange Pos;
        TVector<TUtf16String> Parts;

        TField() {
            Y_ASSERT(!Pos);
        }

        TUtf16String GetName() const {
            const wchar16 delim = ' ';
            return JoinStrings(Parts, TWtringBuf(&delim, 1));
        }

        bool operator !() const {
            return !Pos || Parts.empty();
        }

        bool operator ==(const TField& rhs) const {
            return Pos == rhs.Pos && Parts == rhs.Parts;
        }
    };

public:
    EFIOType FioType;
    TLangMask Language;
    TField FirstNameField;
    TField SurnameField;
    bool IsSurnameReliable = false;
    TVector<TField> SecondNameFields;

public:
    TFioMarkup()
    {}
    TUtf16String GetFirstName() const {
        return FirstNameField.GetName();
    }
    TUtf16String GetSurname() const {
        return SurnameField.GetName();
    }
    TUtf16String GetSecondName(size_t id = 0) const {
        return id < SecondNameFields.size() ? SecondNameFields[id].GetName() : TUtf16String();
    }
    bool Merge(TMarkupDataBase& newNode) override {
        const TFioMarkup& newFio = newNode.As<TFioMarkup>();
        return newFio == *this;
    }
    bool operator ==(const TFioMarkup& rhs) const {
        return FioType == rhs.FioType
            && FirstNameField == rhs.FirstNameField
            && SurnameField == rhs.SurnameField
            && SecondNameFields == rhs.SecondNameFields
            && IsSurnameReliable == rhs.IsSurnameReliable;
    }
    NSearchQuery::TMarkupDataPtr Clone() const override {
        return new TFioMarkup(*this);
    }
    bool Serialize(NRichTreeProtocol::TMarkupDataBase& message, bool humanReadable) const override;
    static NSearchQuery::TMarkupDataPtr Deserialize(const NRichTreeProtocol::TMarkupDataBase& message);
protected:
    bool DoEqualTo(const TMarkupDataBase& rhs) const override {
        return *this == rhs.As<TFioMarkup>();
    }
};

namespace NSearchQuery {
    struct TFIOLanguageCheck {
        const TLangMask Langs;

        explicit TFIOLanguageCheck(const TLangMask& langs)
            : Langs(langs)
        {
        }

        template<class TMarkupItem>
        bool operator () (const TMarkupItem& itm) {
            return itm.template GetDataAs<TFioMarkup>().Language.HasAny(Langs);
        }
    };
}
