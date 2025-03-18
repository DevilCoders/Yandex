#pragma once

#include "visitor.h"
#include <library/cpp/html/blob/document.pb.h>

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>

#include <functional>

namespace NHtml {
    namespace NBlob {
        using TDocumentRef = TIntrusivePtr<class TDocument>;

        struct TDocumentOptions {
            //! Загружать ли значение атрибутов при загрузке dom-дерева.
            bool EnableAttributes = true;
            //! Загружать ли значение стилей при загрузке dom-дерева.
            bool EnableStyles = true;

            inline TDocumentOptions& SetEnableAttributes(bool value) {
                EnableAttributes = value;
                return *this;
            }

            inline TDocumentOptions& SetEnableStyles(bool value) {
                EnableStyles = value;
                return *this;
            }
        };

        class TDocument: public TAtomicRefCount<TDocument> {
        public:
            class TFrame {
            public:
                TFrame(TDocument* doc, const TDocumentPack::TFrame& meta);

                //! Обойти html-дерево данного фрейма.
                bool EnumerateHtmlTree(INodeVisitor* visitor) const;

                //! Идентификатор данного фрейма.
                //!
                //! Идентификатор главного фрейм документа
                //! всегда имеет значение "main".
                TString GetId() const;

                //! Урл данного фрейма.
                TString GetUrl() const;

            private:
                TDocument* Document_;
                TDocumentPack::TFrame Meta_;
            };

        public:
            TDocument();
            ~TDocument();

            //! Обойти html-дерево главного фрейма документа.
            bool EnumerateHtmlTree(INodeVisitor* visitor) const;

            //! Обойти html-дерево указанного фрейма.
            bool EnumerateHtmlTree(INodeVisitor* visitor, const TString& frameId) const;

            //! Обойти дерево фреймов документа.
            void EnumerateFrames(std::function<void(const TFrame&)> cb) const;

            //! Высота видимой области документа.
            ui32 GetHeight() const;

            //! Фактор масштабирования контента документа.
            float GetScaleFactor() const;

            //! Урл главного фрейма (документа).
            TString GetUrl() const;

            //! Ширина видимой области документа.
            ui32 GetWidth() const;

        public:
            //! Загрузить документ из файла.
            static TDocumentRef FromFile(
                const TString& path,
                const TDocumentOptions& opt = TDocumentOptions());

            //! Загрузить документ из строки.
            static TDocumentRef FromString(
                const TString& data,
                const TDocumentOptions& opt = TDocumentOptions());

            //! Загрузить документ из blob-a.
            static TDocumentRef FromBlob(
                const TBlob& data,
                const TDocumentOptions& opt = TDocumentOptions());

        private:
            TDocument(const TBlob& data, const TDocumentOptions& opts);

            bool EnumerateHtmlTreeImpl(const TStringBuf& data, INodeVisitor* visitor) const;

            bool LoadFrames();

            ptrdiff_t LoadIndexMap(const char* p,
                                   const TVector<TString>& names,
                                   const TVector<TString>& values,
                                   TVector<TElement::TPair>* map) const;

            ptrdiff_t LoadIndexMap(const char* p, TVector<std::pair<i32, i32>>* map) const;

            //! Получить текущий абсолютный адрес указанного диапазона.
            TStringBuf Rebase(const TRange& range) const;

            ptrdiff_t SkipIndexMap(const char* p) const;

            bool UpdateStrings();

        private:
            using TFrameHash = THashMap<TString, std::pair<TFrame, TStringBuf>>;

            const TBlob Data_;
            TDocumentPack Meta_;
            //! Карта фреймов данного документа.
            TFrameHash Frames_;

            TVector<TString> AttrNames_;
            TVector<TString> AttrValues_;
            TVector<TString> Tags_;
            TVector<TString> Texts_;
            TVector<TString> StyleNames_;
            TVector<TString> StyleValues_;

            const bool EnableAttributes_;
            const bool EnableStyles_;
        };

    }
}
