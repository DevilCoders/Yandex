#pragma once

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/handler.h>

#include <library/cpp/langmask/langmask.h>

#include <library/cpp/tokenizer/tokenizer.h>

#include <library/cpp/langs/langs.h>

namespace NLemmer {
    class TLemmerHandler: public ITokenHandler {
    public:
        struct TParams {
            TLangMask LangMask;
            const ELanguage* DocLangs;
            NLemmer::TAnalyzeWordOpt WordOpt;

            /*!@param[in] langMask Which morphologies to use for lemmatization.
             * @param[in] docLangs Morphologies preferences. Null-terminated (like C-string).
             * @param[in] Other options.
             * @note If docLangs is not null will try to apply morphologies in order defined by
             * docLangs and will stop on the first successfull application.
             * Don't forget to add explicict PPERDIR on necessary languages to your project,
             * or add kernel/lemmer to use all available languages implicitly
             */
            TParams(const TLangMask& langMask,
                    const ELanguage* docLangs = nullptr,
                    const NLemmer::TAnalyzeWordOpt& opt = NLemmer::TAnalyzeWordOpt())
                : LangMask(langMask)
                , DocLangs(docLangs)
                , WordOpt(opt)
            {
            }
        };

    public:
        TLemmerHandler(const TParams& params, ILemmerResultsHandler& handler);

        void OnToken(const TWideToken& tok, size_t /*origleng*/, NLP_TYPE type) override;

        void SetLangMask(const TLangMask& langMask);

        void SetParams(const TParams& params);
        TParams GetParams() const;

        void Flush() {
            Handler.Flush();
        }

    private:
        TParams Params;
        ILemmerResultsHandler& Handler;
    };
}
