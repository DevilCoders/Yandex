#pragma once

#include <util/generic/strbuf.h>

namespace NAntiRobot {

/**
 * If the host name given in the parameter @a host doesn't correspond to the host type
 * HOST_XMLSEARCH_COMMON (i.e. IsXmlSearchHost(host) == false) returns @a host. Otherwise
 * returns non-XML host which the given one corresponds to. Examples:
 *  - NoXmlSearch("xmlsearch.yandex.ru") == "yandex.ru"
 *  - NoXmlSearch("video-xmlsearch.yandex.com.tr") == "yandex.com.tr"
 *  - NoXmlSearch("xmlsearch.hamster.yandex.com.tr") == "hamster.yandex.com.tr"
 *
 * @see IsXmlSearchHost, EHostType
 */
TStringBuf NoXmlSearch(const TStringBuf& host);

}
