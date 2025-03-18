#pragma once

#include <kernel/snippets/idl/snippets.pb.h>
#include <util/generic/fwd.h>

namespace NSnippets
{

void SetInfoRequestParams(const TStringBuf& infoRequestString, NSnippets::NProto::TSnipReqParams& res);

}
