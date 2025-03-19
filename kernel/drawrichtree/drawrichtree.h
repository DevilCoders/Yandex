#pragma once

#include <library/cpp/charset/doccodes.h>
#include <util/generic/fwd.h>

class IOutputStream;
class IInputStream;

namespace NDrawRichTree {

void GenerateGraphViz (const TString& qtree, IOutputStream& out, ECharset encoding);
void GenerateGraphViz (IInputStream& in, IOutputStream& out, ECharset encoding);

}

