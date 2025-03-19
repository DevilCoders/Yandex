#include "question.h"
#include "extended_length.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/custom/schemaorg_viewer/schemaorg_viewer.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/read_helper/read_helper.h>
#include <kernel/snippets/schemaorg/question/question.h>
#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>
#include <kernel/snippets/smartcut/clearchar.h>
#include <kernel/snippets/strhl/goodwrds.h>

#include <library/cpp/json/json_writer.h>

#include <util/generic/string.h>
#include <util/string/strip.h>
#include <util/string/subst.h>
#include <library/cpp/string_utils/url/url.h>

namespace NSnippets {

    static const char* const HEADLINE_SRC = "question";

    static constexpr size_t MIN_SNIP_LEN = 50;
    static constexpr size_t TOP_ANSWER_COUNT = 3;
    static constexpr size_t TOUCH_QUESTION_LINES = 4;
    static constexpr size_t TOUCH_ANSWER_LINES = 8;
    static constexpr float SQUEEZE_RATE = 2.5f;
    static constexpr size_t TOUCH_EXTENDED_ANSWER_LINES = 18;

    TQuestionReplacer::TQuestionReplacer(const TSchemaOrgArchiveViewer& arcViewer)
        : IReplacer("question")
        , ArcViewer(arcViewer)
    {
    }

    static TMultiCutResult CutWithExt(const TUtf16String& text, const TReplaceContext& ctx, float maxLen, float maxExtLen) {
        if (!ctx.Cfg.UseTouchSnippetBody()) {
            maxLen /= SQUEEZE_RATE;
            maxExtLen /= SQUEEZE_RATE;
        }
        return SmartCutWithExt(text, ctx.IH, maxLen, maxExtLen, TSmartCutOptions(ctx.Cfg));
    }

    static void Cut(TUtf16String& text, const TReplaceContext& ctx, float maxLen) {
        text = CutWithExt(text, ctx, maxLen, 0).Short;
    }

    static void CutAndHilite(TUtf16String& text, const TReplaceContext& ctx, float maxLen) {
        Cut(text, ctx, maxLen);
        ctx.IH.PaintPassages(text, TPaintingOptions::DefaultSnippetOptions());
    }

    static void AddSideblock(TReplaceManager* const manager,
        const TReplaceContext& ctx,
        const NSchemaOrg::TQuestion* const question)
    {
        TUtf16String questionText(question->QuestionText);
        ClearChars(questionText);
        Cut(questionText, ctx, TOUCH_QUESTION_LINES);
        if (questionText.empty()) {
            manager->ReplacerDebug("schemaorg question is empty");
            return;
        }
        TVector<const NSchemaOrg::TAnswer*> answers;
        question->GetAllNotEmptyAnswers(answers);
        if (answers.size() == 0) {
            manager->ReplacerDebug("There is no schemaorg answers");
            return;
        }
        NJson::TJsonValue topAnswers(NJson::JSON_ARRAY);
        size_t maxLen = TOP_ANSWER_COUNT * TOUCH_ANSWER_LINES / Min(answers.size(), TOP_ANSWER_COUNT);
        for (const NSchemaOrg::TAnswer* ans : answers) {
            TUtf16String answerText = ans->Text;
            CutAndHilite(answerText, ctx, maxLen);
            if (answerText.empty()) {
                continue;
            }
            NJson::TJsonValue topAnswer(WideToUTF8(answerText));
            topAnswers.AppendValue(topAnswer);
            if (topAnswers.GetArray().size() >= TOP_ANSWER_COUNT) {
                break;
            }
        }
        if (topAnswers.GetArray().size() == 0) {
            manager->ReplacerDebug("There is no schemaorg answers after CutAndHilite");
            return;
        }
        NJson::TJsonValue features(NJson::JSON_MAP);
        features["type"] = "sideblock_questions";
        features["url"] = ctx.Url;
        features["answers"] = topAnswers;
        features["question"] = WideToUTF8(questionText);
        NJson::TJsonValue data(NJson::JSON_MAP);
        data["features"] = features;
        data["block_type"] = "construct";
        data["content_plugin"] = true;
        data["__has_text"] = false;
        manager->GetExtraSnipAttrs().AddClickLikeSnipJson("sideblock_questions", NJson::WriteJson(data));
    }


    static void AddSpecialSnippet(TReplaceManager* const manager,
        const TReplaceContext& ctx,
        const NSchemaOrg::TQuestion* const question,
        const NSchemaOrg::TAnswer& answer)
    {
        TUtf16String questionText(question->QuestionText);
        ClearChars(questionText);
        CutAndHilite(questionText, ctx, TOUCH_QUESTION_LINES);
        TMultiCutResult cuttedAnswer = CutWithExt(answer.Text, ctx, TOUCH_ANSWER_LINES, TOUCH_EXTENDED_ANSWER_LINES);
        ctx.IH.PaintPassages(cuttedAnswer.Short, TPaintingOptions::DefaultSnippetOptions());
        ctx.IH.PaintPassages(cuttedAnswer.Long, TPaintingOptions::DefaultSnippetOptions());
        NJson::TJsonValue features(NJson::JSON_MAP);
        features["type"] = "snip-question-schema";
        features["answer"] = WideToUTF8(cuttedAnswer.Short);
        features["question"] = WideToUTF8(questionText);
        features["answer_count"] = WideToUTF8(question->AnswerCount);
        if (!ctx.Cfg.ExpFlagOff("mail_ru_extended") && cuttedAnswer.Short.size() < cuttedAnswer.Long.size()) {
            features["answer_extended"] = WideToUTF8(cuttedAnswer.Long);
        }
        NJson::TJsonValue data(NJson::JSON_MAP);
        data["features"] = features;
        data["block_type"] = "construct";
        data["content_plugin"] = true;
        manager->GetExtraSnipAttrs().AddClickLikeSnipJson("schema_question", NJson::WriteJson(data));
    }

    void TQuestionReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& ctx = manager->GetContext();

        const NSchemaOrg::TQuestion* question = ArcViewer.GetQuestion();
        if (!question) {
            return;
        }

        if (ctx.Cfg.IsMainPage()) {
            manager->ReplacerDebug("the main page is not processed");
            return;
        }

        if (!question->AcceptedAnswer.Text && !question->SuggestedAnswer) {
            manager->ReplacerDebug("Question/suggestedAnswer and Question/acceptedAnswer/text fields are missing");
            return;
        }

        NSchemaOrg::TAnswer answer = question->GetBestAnswer();
        question->CleanAnswer(answer.Text);

        const bool approvedMailRuAnswer = GetOnlyHost(ctx.Url) == "otvet.mail.ru" &&
            question->HasApprovedAnswer();
        if (approvedMailRuAnswer &&
            ctx.Cfg.IsTouchReport() &&
            !ctx.Cfg.ExpFlagOff("question_info"))
        {
            AddSpecialSnippet(manager, ctx, question, answer);
        }

        if (ctx.Cfg.ExpFlagOn("sideblock_questions")) {
            AddSideblock(manager, ctx, question);
        }

        const bool bigSnippet = approvedMailRuAnswer && ctx.Cfg.QuestionSnippets();
        const bool forbidShortAnswer = !bigSnippet || ctx.Cfg.QuestionSnippetsRelevant();
        if (answer.Text.size() < MIN_SNIP_LEN && forbidShortAnswer) {
            manager->ReplacerDebug("best answer is too short or empty", TReplaceResult().UseText(answer.Text, HEADLINE_SRC));
            return;
        }

        bool isAlgoSnip = true;
        const float maxSpecSnipLen = bigSnippet ? ctx.Cfg.QuestionSnippetLen() : ctx.LenCfg.GetMaxTextSpecSnipLen();
        const float maxExtLen = GetExtSnipLen(ctx.Cfg, ctx.LenCfg);
        TMultiCutResult extDesc;
        if (!bigSnippet) {
            extDesc = CutSnipWithExt(answer.Text, ctx, maxSpecSnipLen, maxExtLen,
                    ctx.LenCfg.GetMaxSnipLen(), nullptr, ctx.SnipWordSpanLen);
        }
        if (bigSnippet || extDesc.Short.size() < MIN_SNIP_LEN) {
            isAlgoSnip = false;
            extDesc = SmartCutWithExt(answer.Text, ctx.IH, maxSpecSnipLen, maxExtLen, TSmartCutOptions(ctx.Cfg));
        }
        TUtf16String newSnip = extDesc.Short;

        TReplaceResult res;
        res.UseText(extDesc, HEADLINE_SRC);

        if (newSnip.size() < MIN_SNIP_LEN && forbidShortAnswer) {
            manager->ReplacerDebug("headline is too short or empty", res);
            return;
        }

        TReadabilityChecker checker(ctx.Cfg, ctx.QueryCtx);
        checker.CheckBadCharacters = true;
        if (!bigSnippet && !checker.IsReadable(newSnip, isAlgoSnip)) {
            manager->ReplacerDebug("headline is not readable", res);
            return;
        }

        if (!bigSnippet) {
            double oldWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true).Add(ctx.SuperNaturalTitle).Add(ctx.Snip).GetWeight();
            double newWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true).Add(ctx.NaturalTitle).Add(newSnip).GetWeight();
            if (newWeight < oldWeight) {
                manager->ReplacerDebug("headline has not enough non stop query words", res);
                return;
            }
        }
        manager->Commit(res, MRK_QUESTION);
    }

} // namespace NSnippets
