#pragma once

#include <library/cpp/html/face/onchunk.h>
#include <util/generic/ptr.h>

namespace NHtml {
    namespace NBlob {
        /**
         * Вспомагательный класс для упаковки событий html-парсера
         * в формат dom-дерева.
         */
        class TPackEvents: public IParserResult {
        public:
            TPackEvents();
            ~TPackEvents() override;

            //! Возвращает упакованное представление dom-дерева.
            TString Pack() const;

            THtmlChunk* OnHtmlChunk(const THtmlChunk& chunk) override;

        private:
            class TImpl;
            THolder<TImpl> Impl_;
        };

    }
}
