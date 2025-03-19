#include "numerator_utils.h"

namespace NSegutils {

void THtmlDocument::Clear() {
    Url.clear();
    Html.clear();
    Time.SetTimestamp(0);
    HttpCharset = CODES_UNKNOWN;
    ForcedCharset = CODES_UNKNOWN;
    HttpMime = MIME_HTML;
    GuessedCharset = CODES_UNKNOWN;

    HttpLanguage.clear();
    ForcedLanguage.clear();
    GuessedLanguages.clear();
    Errors.clear();
}

}
