#include "jsonp.h"

#include "fullreq_info.h"

#include <library/cpp/regex/regexp_classifier/regexp_classifier.h>

#include <library/cpp/cgiparam/cgiparam.h>

namespace NAntiRobot {

const TStringBuf CGI_CALLBACK("callback");

bool NeedJSONP(const TCgiParameters& cgi) {
    return !cgi.Get(CGI_CALLBACK).empty();
}

TString ToJSONP(const TCgiParameters& cgi, const TStringBuf& json) {
    Y_ASSERT(NeedJSONP(cgi));

    if (!IsCorrectJsonpCallback(cgi)) {
      ythrow TFullReqInfo::TBadRequest() << "Incorrect JSONP callback, cgi: " << cgi.Print();
    }

    return TString::Join("/**/ ",  cgi.Get(CGI_CALLBACK), "(", json, ")");
}

// CAPTCHA-880
bool IsCorrectJsonpCallback(const TCgiParameters& cgi) {
    static const TRegexpClassifier<bool> CallbackClassifier({{"[a-zA-Z0-9_]+", true}}, false);
    return CallbackClassifier[cgi.Get(CGI_CALLBACK)];
}

}
