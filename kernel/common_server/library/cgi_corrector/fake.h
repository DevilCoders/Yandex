#pragma once

#include "abstract.h"

class TFakeCgiCorrector: public ICgiCorrector {
public:
    virtual void FormContextCgi(IRequestContext* /*requestContext*/, ISearchContext** /*searchContext*/) {}
    virtual void FormCgi(TCgiParameters& /*cgi*/, const TBaseServerRequestData* /*rd*/) {}
    virtual void SetClientCgi(const TCgiParameters& /*requestCgi*/, TCgiParameters& /*clientCgi*/, const TRequestParams& /*rp*/) {}
};
