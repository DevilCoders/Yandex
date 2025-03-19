#pragma once

#include <kernel/relev_locale/protos/serptld.pb.h>

#include <util/generic/strbuf.h>
#include <util/string/cast.h> // arcadia style casting is available too

/* Check if `tld` is in CIS (Commonwealth of Independent States) aka СНГ (Содружество Независимых
 * Государств).
 */
inline bool IsCISSerp(const EYandexSerpTld tld) {
    return (tld == YST_RU || tld == YST_UA || tld == YST_BY || tld == YST_KZ || tld == YST_UZ);
}

/* Convert string representation of Yandex TLD (top level domain) into corresponding enum.
 *
 * @note            Empty string will be converted into `YST_RU`, not `YST_UNKNOWN`.
 */
EYandexSerpTld EYandexSerpTldFromString(const TStringBuf& name);

/* Convert Yandex TLD (id) to Yandex domain (first or second level domain). For example:
*   YST_RU -> "ru"
*   YST_TR -> "com.tr"
*   YST_IL -> "co.il"
* @note  For second level domain string representations of Yandex TLD differ from Yandex domains.
         For example:  ToString(YST_TR) == "tr",
                       GetyandexDomainBySerpTld(YST_TR) == "com.tr"
*/
TString GetYandexDomainBySerpTld(EYandexSerpTld tld);
