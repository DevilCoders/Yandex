#include "attribute_fetcher.h"
#include "datefetcher.h"
#include "textfetcher.h"
#include "boolfetcher.h"

namespace NHtmlXPath {

const char* TDateAttributeFetcher::NORMALIZED_DATE_ATTR_POSTFIX = "_normalized";
const char* TDateAttributeFetcher::RAW_DATE_ATTR_POSTFIX = "_raw";

const char* IAttributeValueFetcher::TYPE_ATTRIBUTE = "type";
const char* IAttributeValueFetcher::NAME_ATTRIBUTE = "name";

const char* TTextAttributeFetcher::SEPARATOR_ATTRIBUTE = "join-by";
const char* TTextAttributeFetcher::REGEXP_ATTRIBUTE = "regexp";
const char* TTextAttributeFetcher::ALLOW_EMPTY_ATTRIBUTE = "allow-empty";

const char* TBooleanAttributeFetcher::ALLOW_FALSE_ATTRIBUTE = "allow-false";

} // namespace NHtmlXPath

