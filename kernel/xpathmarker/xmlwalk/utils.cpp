#include "utils.h"

#include "xmlwalk.h"


#include <util/string/cast.h>

namespace NHtmlXPath {

TCapture::TCapture(const TString& regexp)
    : MatchStart(nullptr)
    , MatchLength(0)
{
    if (regexp.empty()) {
        ythrow yexception() << "Regexp must not be empty";
    }
    Parser.Reset(new TRegexStringParser(regexp.data(), regexp.size()));
}

bool TCapture::Capture(const TStringBuf& str, TString& result) {
    Parser->Match(*this, str.data(), str.size());
    if (MatchStart != nullptr) {
        result = TString(MatchStart, MatchLength);
        return true;
    }
    return false;
}

void TCapture::operator () (unsigned /* matchNo */, const char* start, size_t length) {
    MatchStart = start;
    MatchLength = length;
}

void ApplyRegexpIfAny(TString& data, const TString& regexp) {
    if (regexp.empty()) {
        return;
    }

    TCapture capture(regexp);
    capture.Capture(TStringBuf(data.data(), data.size()), data);
}

TString SerializeToStroka(const NXml::TxmlXPathObjectPtr attributeData, const TString& separator /*= " "*/, const TString& regexp /*= TString()*/) {
    bool haveRegexp = !!regexp;

    TString result;
    switch (attributeData->type) {
        case XPATH_STRING: {
            result = (const char*) attributeData->stringval;
            break;
        }
        case XPATH_NUMBER: {
            result = ToString(attributeData->floatval);
            break;
        }
        case XPATH_BOOLEAN: {
            result = ToString(attributeData->boolval);
            break;
        }
        case XPATH_NODESET: {
            xmlNodeSetPtr nodes = attributeData->nodesetval;
            int nodesSelected = xmlXPathNodeSetGetLength(nodes);

            for (int nodeNo = 0; nodeNo < nodesSelected; ++nodeNo) {
                xmlNodePtr node = xmlXPathNodeSetItem(nodes, nodeNo);

                TString text = NHtml::NXmlWalk::GetNodeTextContent(node);
                if (haveRegexp) {
                    ApplyRegexpIfAny(text, regexp);
                }
                if (!text.empty()) {
                    if (!result.empty()) {
                        result += separator;
                    }
                    result += text;
                }
            }
            break;
        }
        default: {
            // just to supress warnings (we can not process other variants)
            break;
        }
    }
    if (haveRegexp) {
        ApplyRegexpIfAny(result, regexp);
    }
    return result;
}

} // NHtmlXPath

