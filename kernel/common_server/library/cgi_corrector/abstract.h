#pragma once
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/generic/ptr.h>

class IRequestContext;
class ISearchContext;
class TRequestParams;
class TBaseServerRequestData;

class ICgiCorrector
    : public TAtomicRefCount<ICgiCorrector>
    , TNonCopyable
{
public:
    virtual ~ICgiCorrector() {}

    virtual void FormContextCgi(IRequestContext* requestContext, ISearchContext** searchContext) = 0;
    virtual void FormCgi(TCgiParameters& cgi, const TBaseServerRequestData* rd) = 0;
    virtual void SetClientCgi(const TCgiParameters& requestCgi, TCgiParameters& clientCgi, const TRequestParams& rp) = 0;
};

using ICgiCorrectorPtr = TIntrusivePtr<ICgiCorrector>;
