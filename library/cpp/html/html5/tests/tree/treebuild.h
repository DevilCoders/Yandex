#pragma once

#include <util/stream/output.h>

void PrintChunkTree(const TStringBuf& html, IOutputStream& out);
void PrintParserTree(const TStringBuf& html, IOutputStream& out);
