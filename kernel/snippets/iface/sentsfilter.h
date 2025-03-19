#pragma once

namespace NSnippets {

class ISentsFilter  {
public:
    virtual ~ISentsFilter() {}
    virtual bool IsPermitted(int /*docid*/, int /*sentId*/) const = 0;
};

}
