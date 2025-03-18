#include "inforeq.h"

#include <search/reqparam/inforeqparam.h>
#include <search/reqparam/snipreqparam.h>
#include <search/reqparam/reqparam.h>

namespace NSnippets
{

void SetInfoRequestParams(const TStringBuf& infoRequestString, NSnippets::NProto::TSnipReqParams& res)
{
    TInfoParams infoParams;
    infoParams.Init(infoRequestString);
    infoParams.Params.Set("json", "1");
    SetSnipConfInfoRequest(infoParams, res);
}

}
