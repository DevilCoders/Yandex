#pragma once

#include <kernel/doom/info/index_format.h>
#include <kernel/doom/info/index_format_usage.h>

#include <library/cpp/colorizer/colors.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/noncopyable.h>
#include <util/generic/map.h>
#include <util/generic/yexception.h>

#include <functional>

namespace NDoom {

template<typename TProcessor = std::function<void()>>
class TIndexFormatProcessor: public TNonCopyable {
public:
    TIndexFormatProcessor() = default;

    void AddProcessor(EIndexFormat indexFormat, TProcessor&& processor) {
        Y_ENSURE(!Processors_.contains(indexFormat));
        Processors_[indexFormat] = std::move(processor);
    }

    template<typename... TArgs>
    void Process(EIndexFormat indexFormat, TArgs... args) const {
        if (!Processors_.contains(indexFormat)) {
            ythrow yexception() << "Unsupported index format '" << indexFormat << "'.";
        }
        Processors_.at(indexFormat)(std::forward<TArgs>(args)...);
    }

    void PrintValidFormats(IOutputStream& outputStream) const {
        NColorizer::TColors& colors = NColorizer::AutoColors(outputStream);
        outputStream << colors.BoldColor() << "Valid formats:" << colors.OldColor() << Endl;

        for (const auto& processor : Processors_) {
            outputStream << "  ";
            outputStream << colors.GreenColor() << processor.first << colors.OldColor();
            outputStream << " - " << GetIndexFormatUsage(processor.first) << Endl;
        }
    }

    template<typename... TArgs>
    void RegisterHelpHandler(NLastGetopt::TOpts& opts, IOutputStream& outputStream, TArgs... args) const {
        auto helpHandler = [&] (const NLastGetopt::TOptsParser* p) {
            p->PrintUsage(outputStream);
            outputStream << Endl;
            PrintValidFormats(outputStream);
            exit(0);
        };
        opts.AddLongOption(std::forward<TArgs>(args)...).NoArgument().Handler1(helpHandler);
    }

private:
    TMap<EIndexFormat, TProcessor> Processors_;
};

} // namespace NDoom
