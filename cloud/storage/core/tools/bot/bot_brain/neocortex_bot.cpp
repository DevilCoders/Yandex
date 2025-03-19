#include "neocortex_bot.h"

#include <util/random/random.h>
#include <util/stream/file.h>

namespace NCoolBot {

////////////////////////////////////////////////////////////////////////////////

TNeocortexBot::TNeocortexBot(const NJson::TJsonValue& config)
{
    {
        TIFStream is(config["model"].GetStringSafe());
        Pack.Load(&is);
    }

    {
        TIFStream is(config["answers"].GetStringSafe());
        TString line;
        while (is.ReadLine(line)) {
            Answers.push_back(new TAnswer{
                line,
                NNeocortex::ITextClassifier::TContext(Pack.V, line, &RareWords)
            });
        }
    }

    const auto& settings = config["settings"];
    auto getSetting = [&] (const TString& name, double& val) {
        if (settings.Has(name)) {
            val = settings[name].GetDoubleSafe();
        }
    };
    getSetting("DropAnswerProbability", Settings.DropAnswerProbability);
    getSetting("MinAnswerScore", Settings.MinAnswerScore);
}

TNeocortexBot::TResult TNeocortexBot::SelectAnswer(const TString& message)
{
    NNeocortex::ITextClassifier::TContext context(
        Pack.V,
        message,
        &RareWords
    );

    TResult result;

    for (const auto& answer: Answers) {
        if (result.Score >= Settings.MinAnswerScore
                && RandomNumber<double>() < Settings.DropAnswerProbability)
        {
            continue;
        }

        auto score = Pack.Classifier->CalcScore(context, answer->Context);
        if (score > result.Score) {
            result.Score = score;
            result.Text = answer->Text;
        }
    }

    return result;
}

}   // namespace NCoolBot
