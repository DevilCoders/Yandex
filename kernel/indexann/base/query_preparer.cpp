#include "query_preparer.h"

#include <util/charset/wide.h>


using namespace NIndexAnn;

TQueryPreparer::TQueryPreparer(const TConfig& config)
    : Config(config)
{
}

bool TQueryPreparer::EnsureSize(TKeyInvText& result) const
{
    if (result.size() > Config.MaxLength){
        if (Config.Reject){
            return false;
        }
        result.resize(Config.MaxLength);
        const auto& fullWords = result.find_last_of(UTF8ToWide(" "));
        result.resize(fullWords);
    }
    if (result.size() < Config.MinLength) {
        return false;
    }
    return true;
}

bool TQueryPreparer::TryConvert(const TStringBuf& query, TKeyInvText& result) const
{
    try {
        result = UTF8ToWide(query);

        if (!EnsureSize(result)) {
            return false;
        }
    } catch (const yexception& ex) {
        if (Config.Verbose) {
            Cerr << "TQueryPreparer::TryConvert: " << ex.what() << Endl;
        }
        return false;
    }

    return true;
}

TKeyInvText TQueryPreparer::Convert(const TStringBuf& query) const
{
    TKeyInvText res;
    if (!TryConvert(query, res)) {
        ythrow yexception() << "Conversion to TUtf16String failed for '" << query << "'";
    }
    return res;
}

