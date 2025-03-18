#pragma once
#include <util/generic/string.h>
#include <util/generic/yexception.h>

class TCgiParameters;

namespace NAntiRobot {

class TRequest;

/**
 * The class represents cgi-parameter 'retPath' that is used to return the user
 * to SERP after having successfully entered CAPTCHA.
 */
class TReturnPath {
public:
    /**
     * Creates initial return path from the search request given in @arg req.
     *
     * This method should be called when we redirect the user to CAPTCHA
     * for the first time.
     */
    static TReturnPath FromRequest(const TRequest& req);

    /**
     * Creates return path from CGI parameters that are acquired from
     * /showcaptcha and /checkcaptcha requests.
     *
     * @throw TInvalidRetPathException
     */
    static TReturnPath FromCgi(const TCgiParameters& cgi);

    class TInvalidRetPathException : public yexception {
    };

    const TString& GetURL() const {
        return Url;
    }

    void AddToCGI(TCgiParameters& cgi) const;

    static const TStringBuf CGI_PARAM_NAME;

private:
    TReturnPath(const TStringBuf& url);

    TString Url;
};

} /* namespace NAntiRobot */
