#include <util/generic/hash.h>
#include <util/generic/singleton.h>

#include "bytld.h"

struct TTLDAndLanguage {
    const char* TLD;
    ELanguage Language;
};

static const TTLDAndLanguage TLDAndLanguage[] = {
    {"ac", LANG_ENG}, // Ascension Island
    {"ad", LANG_CAT}, // Andorra
    {"ae", LANG_ARA}, // United Arab Emirates
    {"af", LANG_UNK}, // Afghanistan  (Dari, Pashto)
    {"ag", LANG_ENG}, // Antigua and Barbuda
    {"ai", LANG_ENG}, // Anguilla
    {"al", LANG_ALB}, // Albania
    {"am", LANG_ARM}, // Armenia
    {"an", LANG_ENG}, // Netherlands Antilles
    {"ao", LANG_POR}, // Angola
    {"aq", LANG_UNK}, // Antarctica (Penguinese)
    {"ar", LANG_SPA}, // Argentina
    {"as", LANG_ENG}, // American Samoa
    {"at", LANG_GER}, // Austria
    {"au", LANG_ENG}, // Australia
    {"aw", LANG_DUT}, // Aruba
    {"ax", LANG_SWE}, // Aland
    {"az", LANG_UNK}, // Azerbaijan (Azerbaijani)
    {"ba", LANG_HRV}, // Bosnia and Herzegovina (+ Bosnian, Serbian)
    {"bb", LANG_ENG}, // Barbados
    {"bd", LANG_UNK}, // Bangladesh (Bangla)
    {"be", LANG_DUT}, // Belgium (+ French, German)
    {"bf", LANG_FRE}, // Burkina Faso
    {"bg", LANG_BUL}, // Bulgaria
    {"bh", LANG_ARA}, // Bahrain
    {"bi", LANG_FRE}, // Burundi (+ Kirundi)
    {"bj", LANG_FRE}, // Benin
    {"bm", LANG_ENG}, // Bermuda
    {"bn", LANG_UNK}, // Brunei Darussalam (Malay)
    {"bo", LANG_SPA}, // Bolivia (+ Quechua, Aymara, 34 other native languages)
    {"br", LANG_POR}, // Brazil
    {"bs", LANG_ENG}, // Bahamas
    {"bt", LANG_UNK}, // Bhutan (Dzongkha)
    {"bv", LANG_NOR}, // Bouvet Island
    {"bw", LANG_ENG}, // Botswana (+ Setswana)
    {"by", LANG_BEL}, // Belarus (+ Russian)
    {"bz", LANG_ENG}, // Belize
    {"ca", LANG_ENG}, // Canada
    {"cc", LANG_ENG}, // Cocos (Keeling) Islands
    {"cd", LANG_FRE}, // Democratic Republic of the Congo
    {"cf", LANG_FRE}, // Central African Republic (+ Sango)
    {"cg", LANG_FRE}, // Republic of the Congo
    {"ch", LANG_GER}, // Switzerland (+ French, Italian, Romansh)
    {"ci", LANG_FRE}, // Cote d'Ivoire
    {"ck", LANG_ENG}, // Cook Islands
    {"cl", LANG_SPA}, // Chile
    {"cm", LANG_ENG}, // Cameroon (+ French)
    {"cn", LANG_UNK}, // People's Republic of China (Putonghua)
    {"co", LANG_SPA}, // Colombia
    {"cr", LANG_SPA}, // Costa Rica
    {"cs", LANG_CZE}, // Czechoslovakia (+ Slovak)
    {"cu", LANG_SPA}, // Cuba
    {"cv", LANG_POR}, // Cape Verde
    {"cx", LANG_ENG}, // Christmas Island
    {"cy", LANG_GRE}, // Cyprus (+ Turkish)
    {"cz", LANG_CZE}, // Czech Republic
    {"de", LANG_GER}, // Germany
    {"dj", LANG_FRE}, // Djibouti (+ Arabic)
    {"dk", LANG_DAN}, // Denmark
    {"dm", LANG_ENG}, // Dominica
    {"do", LANG_SPA}, // Dominican Republic
    {"dz", LANG_ARA}, // Algeria
    {"ec", LANG_SPA}, // Ecuador
    {"ee", LANG_EST}, // Estonia
    {"eg", LANG_ARA}, // Egypt
    {"er", LANG_ITA}, // Eritrea (+ Tigrinya, Arabic)
    {"es", LANG_SPA}, // Spain
    {"et", LANG_UNK}, // Ethiopia (Amharic)
    {"eu", LANG_UNK}, // European Union (23)
    {"fi", LANG_FIN}, // Finland (+ Sweddish)
    {"fj", LANG_ENG}, // Fiji (+ Bau Fijian, Fiji Hindi)
    {"fk", LANG_ENG}, // Falkland Islands
    {"fm", LANG_ENG}, // Federated States of Micronesia
    {"fo", LANG_DAN}, // Faroe Islands (+ Faroese)
    {"fr", LANG_FRE}, // France
    {"ga", LANG_FRE}, // Gabon
    {"gb", LANG_ENG}, // United Kingdom
    {"gd", LANG_ENG}, // Grenada
    {"ge", LANG_GEO}, // Georgia
    {"gf", LANG_FRE}, // French Guiana
    {"gg", LANG_ENG}, // Guernsey
    {"gh", LANG_ENG}, // Ghana
    {"gi", LANG_ENG}, // Gibraltar
    {"gl", LANG_UNK}, // Greenland (Greenlandic)
    {"gm", LANG_ENG}, // The Gambia
    {"gn", LANG_FRE}, // Guinea
    {"gp", LANG_FRE}, // Guadeloupe
    {"gq", LANG_SPA}, // Equatorial Guinea (+ French)
    {"gr", LANG_GRE}, // Greece
    {"gs", LANG_ENG}, // South Georgia and the South Sandwich Islands
    {"gt", LANG_SPA}, // Guatemala
    {"gu", LANG_ENG}, // Guam
    {"gw", LANG_POR}, // Guinea-Bissau
    {"gy", LANG_ENG}, // Guyana
    {"hk", LANG_ENG}, // Hong Kong (+ Chinese)
    {"hm", LANG_ENG}, // Heard Island and McDonald Islands
    {"hn", LANG_SPA}, // Honduras
    {"hr", LANG_HRV}, // Croatia
    {"ht", LANG_FRE}, // Haiti (+ Haitian, Creole)
    {"hu", LANG_HUN}, // Hungary
    {"id", LANG_UNK}, // Indonesia (Indonesian)
    {"ie", LANG_ENG}, // Ireland (+ Irish)
    {"il", LANG_HEB}, // Israel (+ Arabic)
    {"im", LANG_ENG}, // Isle of Man (+ Manx)
    {"in", LANG_ENG}, // India (+ Hindi)
    {"io", LANG_ENG}, // British Indian Ocean Territory
    {"iq", LANG_ARA}, // Iraq (+ Kurdish)
    {"ir", LANG_PER}, // Iran
    {"is", LANG_UNK}, // Iceland (Icelandic)
    {"it", LANG_ITA}, // Italy
    {"je", LANG_ENG}, // Jersey (+ French)
    {"jm", LANG_ENG}, // Jamaica
    {"jo", LANG_ARA}, // Jordan
    {"jp", LANG_UNK}, // Japan (Japanese)
    {"ke", LANG_ENG}, // Kenya (+ Swahili)
    {"kg", LANG_KIR}, // Kyrgyzstan (+ Russian)
    {"kh", LANG_UNK}, // Cambodia (Khmer)
    {"ki", LANG_ENG}, // Kiribati (+ Gilbertese)
    {"km", LANG_FRE}, // Comoros (+ Comorian, Arabic)
    {"kn", LANG_ENG}, // Saint Kitts and Nevis
    {"kp", LANG_KOR}, // Democratic People's Republic of Korea
    {"kr", LANG_KOR}, // Republic of Korea
    {"kw", LANG_ARA}, // Kuwait
    {"ky", LANG_ENG}, // Cayman Islands
    {"kz", LANG_KAZ}, // Kazakhstan (+ Russian)
    {"la", LANG_UNK}, // Laos (Lao)
    {"lb", LANG_ARA}, // Lebanon (+ French)
    {"lc", LANG_FRE}, // Saint Lucia
    {"li", LANG_GER}, // Liechtenstein
    {"lk", LANG_UNK}, // Sri Lanka (Sinhala, Tamil)
    {"lr", LANG_ENG}, // Liberia
    {"ls", LANG_ENG}, // Lesotho (+ Sesotho)
    {"lt", LANG_LIT}, // Lithuania
    {"lu", LANG_FRE}, // Luxembourg (+ Luxembourgish, German)
    {"lv", LANG_LAV}, // Latvia
    {"ly", LANG_ARA}, // Libya
    {"ma", LANG_ARA}, // Morocco
    {"mc", LANG_FRE}, // Monaco
    {"md", LANG_RUM}, // Moldova
    {"me", LANG_SRP}, // Montenegro (+ Montenegrin)
    {"mg", LANG_FRE}, // Madagascar (+ Malagasy)
    {"mh", LANG_ENG}, // Marshall Islands (+ Marshallese)
    {"mk", LANG_MAC}, // Republic of Macedonia
    {"ml", LANG_FRE}, // Mali
    {"mm", LANG_UNK}, // Myanmar (Burmese)
    {"mn", LANG_MON}, // Mongolia
    {"mo", LANG_POR}, // Macau (+ Chinese)
    {"mp", LANG_ENG}, // Northern Mariana Islands (+ Chamorro, Carolinian)
    {"mq", LANG_FRE}, // Martinique
    {"mr", LANG_ARA}, // Mauritania
    {"ms", LANG_ENG}, // Montserrat
    {"mt", LANG_ENG}, // Malta (+ Maltese)
    {"mu", LANG_ENG}, // Mauritius
    {"mv", LANG_UNK}, // Maldives (Dhivehi)
    {"mw", LANG_ENG}, // Malawi (+ Chichewa)
    {"mx", LANG_SPA}, // Mexico
    {"my", LANG_ENG}, // Malaysia (Bahasa Malaysia)
    {"mz", LANG_POR}, // Mozambique
    {"na", LANG_ENG}, // Namibia
    {"nc", LANG_FRE}, // New Caledonia
    {"ne", LANG_FRE}, // Niger
    {"nf", LANG_ENG}, // Norfolk Island (+ Norfuk)
    {"ng", LANG_ENG}, // Nigeria
    {"ni", LANG_SPA}, // Nicaragua
    {"nl", LANG_DUT}, // Netherlands
    {"no", LANG_NOR}, // Norway
    {"np", LANG_UNK}, // Nepal (Nepali)
    {"nr", LANG_ENG}, // Nauru (+ Nauruan)
    {"nu", LANG_ENG}, // Niue (+ Niuean)
    {"nz", LANG_ENG}, // New Zealand (+ Maori)
    {"om", LANG_ARA}, // Oman
    {"pa", LANG_SPA}, // Panama
    {"pe", LANG_SPA}, // Peru
    {"pf", LANG_FRE}, // French Polynesia
    {"pg", LANG_ENG}, // Papua New Guinea (+ Tok Pisin, Hiri Motu)
    {"ph", LANG_ENG}, // Philippines (+ Filipino)
    {"pk", LANG_ENG}, // Pakistan (+ Urdu)
    {"pl", LANG_POL}, // Poland
    {"pm", LANG_FRE}, // Saint-Pierre and Miquelon
    {"pn", LANG_ENG}, // Pitcairn Islands
    {"pr", LANG_SPA}, // Puerto Rico (+ English)
    {"ps", LANG_ARA}, // Palestinian territories
    {"pt", LANG_POR}, // Portugal
    {"pw", LANG_ENG}, // Palau (+ Palauan)
    {"py", LANG_SPA}, // Paraguay (+ Guarani)
    {"qa", LANG_ARA}, // Qatar
    {"re", LANG_FRE}, // Reunion
    {"ro", LANG_RUM}, // Romania
    {"rs", LANG_SRP}, // Serbia
    {"ru", LANG_RUS}, // Russia
    {"rw", LANG_FRE}, // Rwanda (+ Kinyarwanda, English)
    {"sa", LANG_ARA}, // Saudi Arabia
    {"sb", LANG_ENG}, // Solomon Islands
    {"sc", LANG_FRE}, // Seychelles (+ English, Seychellois Creole)
    {"sd", LANG_ARA}, // Sudan (+ English)
    {"se", LANG_SWE}, // Sweden
    {"sg", LANG_ENG}, // Singapore (+ Malay, Chinese, Tamil)
    {"sh", LANG_ENG}, // Saint Helena
    {"si", LANG_SLV}, // Slovenia
    {"sj", LANG_NOR}, // Svalbard and  Jan Mayen Islands
    {"sk", LANG_SLO}, // Slovakia
    {"sl", LANG_ENG}, // Sierra Leone
    {"sm", LANG_ITA}, // San Marino
    {"sn", LANG_FRE}, // Senegal
    {"so", LANG_ARA}, // Somalia (+ Somali)
    {"sr", LANG_DUT}, // Suriname
    {"st", LANG_POR}, // Sao Tome and Principe
    {"su", LANG_RUS}, // Soviet Union
    {"sv", LANG_SPA}, // El Salvador
    {"sy", LANG_ARA}, // Syria
    {"sz", LANG_ENG}, // Swaziland (+ Siswati)
    {"tc", LANG_ENG}, // Turks and Caicos Islands
    {"td", LANG_FRE}, // Chad (+ Arabic)
    {"tf", LANG_FRE}, // French Southern and Antarctic Lands
    {"tg", LANG_FRE}, // Togo
    {"th", LANG_UNK}, // Thailand (Thai)
    {"tj", LANG_TGK}, // Tajikistan
    {"tk", LANG_ENG}, // Tokelau (+ Tokelauan)
    {"tl", LANG_POR}, // East Timor (+ Tetum)
    {"tm", LANG_TUK}, // Turkmenistan
    {"tn", LANG_ARA}, // Tunisia
    {"to", LANG_ENG}, // Tonga (+ Tongan)
    {"tp", LANG_POR}, // East Timor (+ Tetum)
    {"tr", LANG_TUR}, // Turkey
    {"tt", LANG_ENG}, // Trinidad and Tobago
    {"tv", LANG_ENG}, // Tuvalu (+ Tuvaluan)
    {"tw", LANG_UNK}, // Republic of China (Taiwan) (Mandarin)
    {"tz", LANG_ENG}, // Tanzania (+ Swahili)
    {"ua", LANG_UKR}, // Ukraine
    {"ug", LANG_ENG}, // Uganda (+ Swahili)
    {"uk", LANG_ENG}, // United Kingdom
    {"us", LANG_ENG}, // United States of America
    {"uy", LANG_SPA}, // Uruguay
    {"uz", LANG_UZB}, // Uzbekistan
    {"va", LANG_ITA}, // Vatican City
    {"vc", LANG_ENG}, // Saint Vincent and the Grenadines
    {"ve", LANG_SPA}, // Venezuela
    {"vg", LANG_ENG}, // British Virgin Islands
    {"vi", LANG_ENG}, // U.S. Virgin Islands
    {"vn", LANG_UNK}, // Vietnam (Vietnamese)
    {"vu", LANG_ENG}, // Vanuatu (+ Bislama, French)
    {"wf", LANG_FRE}, // Wallis and Futuna
    {"ws", LANG_ENG}, // Samoa (+ Samoan)
    {"ye", LANG_ARA}, // Yemen
    {"yt", LANG_FRE}, // Mayotte
    {"za", LANG_ENG}, // South Africa (+ 10)
    {"zm", LANG_ENG}, // Zambia
    {"zw", LANG_ENG}, // Zimbabwe (+ Shona, Ndebele)

    {"biz", LANG_ENG},
    {"com", LANG_ENG},
    {"edu", LANG_ENG},
    {"info", LANG_ENG},
    {"int", LANG_ENG},
    {"mobi", LANG_ENG},
    {"net", LANG_ENG},
    {"org", LANG_ENG},

    {"рф", LANG_RUS},
    {"xn--p1ai", LANG_RUS},
};

class TTLDLanguageMap {
private:
    typedef THashMap<const char*, ELanguage, THash<const char*>, TEqualTo<const char*>> TTLDsHash;
    TTLDsHash Hash;

private:
    void AddTLD(const char* TLD, ELanguage lang) {
        if (TLD == nullptr || strlen(TLD) == 0)
            return;
        if (Hash.find(TLD) != Hash.end())
            Y_ASSERT(Hash.find(TLD)->second == lang);
        Hash[TLD] = lang;
    }

public:
    TTLDLanguageMap() {
        for (size_t i = 0; i != Y_ARRAY_SIZE(TLDAndLanguage); ++i)
            AddTLD(TLDAndLanguage[i].TLD, TLDAndLanguage[i].Language);
    }

    inline ELanguage LanguageByTLD(const char* TLD) const {
        if (!TLD)
            return LANG_UNK;
        TTLDsHash::const_iterator i = Hash.find(TLD);
        if (i == Hash.end())
            return LANG_UNK;
        return i->second;
    }
};

TString TLDByHost(const TString& host) {
    TString tld = host;
    size_t p = tld.rfind('.');
    if (p != TString::npos)
        tld = host.substr(p + 1);
    return to_lower(tld);
}

ELanguage LanguageByTLD(const TString& tld) {
    return Singleton<TTLDLanguageMap>()->LanguageByTLD(tld.data());
}
