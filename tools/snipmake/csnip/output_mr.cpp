#include "output_mr.h"

#include <library/cpp/digest/sfh/sfh.h>

#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/stream/output.h>

namespace NSnippets {

    static TString GetRawInputHash(const TContextData& ctx) {
        const TString& data = ctx.GetRawInput();
        return ToString(SuperFastHash(data.data(), data.size()));
    }

    void TMRConverter::Convert(const TContextData& contextData) {
        TContextData ctx(contextData);
        if (!ctx.ParseToProtobuf())
            return;
        Cout << GetRequestId(ctx) << "\t" <<
                GetRawInputHash(ctx) << "\t" <<
                ctx.GetRawInput() << Endl;
    }

} //namespace NSnippets
