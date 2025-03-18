#pragma once

#include "event.h"

class IParserResult {
public:
    virtual ~IParserResult() {
    }
    virtual THtmlChunk* OnHtmlChunk(const THtmlChunk& chunk) = 0;
};
