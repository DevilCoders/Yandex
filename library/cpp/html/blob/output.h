#pragma once

#include <library/cpp/html/blob/document.pb.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

namespace NHtml {
    namespace NBlob {
        class TOutput {
        public:
            TOutput(IOutputStream* slave);
            ~TOutput();

            ui32 Position() const;

            void WriteUInt8(ui8 value);

            void WriteVariableInt32(i32 value);

            void WriteBytes(const void* data, int len);

            void WriteString(const TString& value);

        private:
            IOutputStream* Slave_;
            ui32 Pos_;
        };

        class TDictionaries {
            using TStringIds = THashMap<TString, i32>;
            using TStringVector = TVector<std::pair<TString, i32>>;

        public:
            inline i32 GetAttrNameId(const TString& name) {
                return GetStringId(name, &AttrNames_);
            }

            inline i32 GetAttrValueId(const TString& name) {
                return GetStringId(name, &AttrValues_);
            }

            inline i32 GetTagId(const TString& name) {
                return GetStringId(name, &TagIds_);
            }

            inline i32 GetStyleNameId(const TString& name) {
                return GetStringId(name, &StyleNames_);
            }

            inline i32 GetStyleValueId(const TString& name) {
                return GetStringId(name, &StyleValues_);
            }

            inline i32 GetTextId(const TString& name) {
                return GetStringId(name, &Texts_);
            }

            void Save(TOutput* output, TDocumentPack::TStrings* index) const;

        private:
            i32 GetStringId(const TString& name, TStringIds* ids);

            TRange WriteStrings(const TStringIds& ids, TOutput* output) const;

        private:
            TStringIds AttrNames_;
            TStringIds AttrValues_;
            TStringIds TagIds_;
            TStringIds StyleNames_;
            TStringIds StyleValues_;
            TStringIds Texts_;
        };

    }
}
