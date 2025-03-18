#include "regcodes.h"

#include <library/cpp/charset/ci_string.h>
#include <util/generic/singleton.h>
#include <util/generic/hash.h>

namespace NRegion {
    const char* const UNKNOWN_REGION = "UNK";

    struct TRegionInfo {
        const char* CountryName;
        ui16 NumCode;
        const char* Latin2Code;
        const char* Latin3Code;
        const char* Synonyms;
    };

    static const TRegionInfo RegionsInfo[] = {
        {"Afghanistan", 4, "AF", "AFG", nullptr},
        {"Aland Islands", 248, "AX", "ALA", nullptr},
        {"Albania", 8, "AL", "ALB", nullptr},
        {"Algeria", 12, "DZ", "DZA", nullptr},
        {"American Samoa", 16, "AS", "ASM", nullptr},
        {"Andorra", 20, "AD", "AND", nullptr},
        {"Angola", 24, "AO", "AGO", nullptr},
        {"Anguilla", 660, "AI", "AIA", nullptr},
        {"Antarctica", 10, "AQ", "ATA", nullptr},
        {"Antigua and Barbuda", 28, "AG", "ATG", nullptr},
        {"Argentina", 32, "AR", "ARG", nullptr},
        {"Armenia", 51, "AM", "ARM", nullptr},
        {"Aruba", 533, "AW", "ABW", nullptr},
        {"Australia", 36, "AU", "AUS", nullptr},
        {"Austria", 40, "AT", "AUT", nullptr},
        {"Azerbaijan", 31, "AZ", "AZE", nullptr},
        {"Bahamas", 44, "BS", "BHS", nullptr},
        {"Bahrain", 48, "BH", "BHR", nullptr},
        {"Bangladesh", 50, "BD", "BGD", nullptr},
        {"Barbados", 52, "BB", "BRB", nullptr},
        {"Belarus", 112, "BY", "BLR", nullptr},
        {"Belgium", 56, "BE", "BEL", nullptr},
        {"Belize", 84, "BZ", "BLZ", nullptr},
        {"Benin", 204, "BJ", "BEN", nullptr},
        {"Bermuda", 60, "BM", "BMU", nullptr},
        {"Bhutan", 64, "BT", "BTN", nullptr},
        {"Bolivia", 68, "BO", "BOL", nullptr},
        {"Bosnia and Herzegovina", 70, "BA", "BIH", nullptr},
        {"Botswana", 72, "BW", "BWA", nullptr},
        {"Bouvet Island", 74, "BV", "BVT", nullptr},
        {"Brazil", 76, "BR", "BRA", nullptr},
        {"British Virgin Islands", 92, "VG", "VGB", nullptr},
        {"British Indian Ocean Territory", 86, "IO", "IOT", nullptr},
        {"Brunei Darussalam", 96, "BN", "BRN", nullptr},
        {"Bulgaria", 100, "BG", "BGR", nullptr},
        {"Burkina Faso", 854, "BF", "BFA", nullptr},
        {"Burundi", 108, "BI", "BDI", nullptr},
        {"Cambodia", 116, "KH", "KHM", nullptr},
        {"Cameroon", 120, "CM", "CMR", nullptr},
        {"Canada", 124, "CA", "CAN", nullptr},
        {"Cape Verde", 132, "CV", "CPV", nullptr},
        {"Cayman Islands", 136, "KY", "CYM", nullptr},
        {"Central African Republic", 140, "CF", "CAF", nullptr},
        {"Chad", 148, "TD", "TCD", nullptr},
        {"Chile", 152, "CL", "CHL", nullptr},
        {"China", 156, "CN", "CHN", nullptr},
        {"Hong Kong", 344, "HK", "HKG", nullptr},
        {"Macao", 446, "MO", "MAC", nullptr},
        {"Christmas Island", 162, "CX", "CXR", nullptr},
        {"Cocos Islands", 166, "CC", "CCK", nullptr},
        {"Colombia", 170, "CO", "COL", nullptr},
        {"Comoros", 174, "KM", "COM", nullptr},
        {"Congo (Brazzaville)", 178, "CG", "COG", nullptr},
        {"Congo, Democratic Republic of the", 180, "CD", "COD", nullptr},
        {"Cook Islands", 184, "CK", "COK", nullptr},
        {"Costa Rica", 188, "CR", "CRI", nullptr},
        {"Ivory Coast", 384, "CI", "CIV", nullptr},
        {"Croatia", 191, "HR", "HRV", nullptr},
        {"Cuba", 192, "CU", "CUB", nullptr},
        {"Cyprus", 196, "CY", "CYP", nullptr},
        {"Czech Republic", 203, "CZ", "CZE", nullptr},
        {"Denmark", 208, "DK", "DNK", nullptr},
        {"Djibouti", 262, "DJ", "DJI", nullptr},
        {"Dominica", 212, "DM", "DMA", nullptr},
        {"Dominican Republic", 214, "DO", "DOM", nullptr},
        {"Ecuador", 218, "EC", "ECU", nullptr},
        {"Egypt", 818, "EG", "EGY", nullptr},
        {"El Salvador", 222, "SV", "SLV", nullptr},
        {"Equatorial Guinea", 226, "GQ", "GNQ", nullptr},
        {"Eritrea", 232, "ER", "ERI", nullptr},
        {"Estonia", 233, "EE", "EST", nullptr},
        {"Ethiopia", 231, "ET", "ETH", nullptr},
        {"Falkland Islands (Malvinas)", 238, "FK", "FLK", nullptr},
        {"Faroe Islands", 234, "FO", "FRO", nullptr},
        {"Fiji", 242, "FJ", "FJI", nullptr},
        {"Finland", 246, "FI", "FIN", nullptr},
        {"France", 250, "FR", "FRA", nullptr},
        {"French Guiana", 254, "GF", "GUF", nullptr},
        {"French Polynesia", 258, "PF", "PYF", nullptr},
        {"French Southern Territories", 260, "TF", "ATF", nullptr},
        {"Gabon", 266, "GA", "GAB", nullptr},
        {"Gambia", 270, "GM", "GMB", nullptr},
        {"Georgia", 268, "GE", "GEO", nullptr},
        {"Germany", 276, "DE", "DEU", "GER"},
        {"Ghana", 288, "GH", "GHA", nullptr},
        {"Gibraltar", 292, "GI", "GIB", nullptr},
        {"Greece", 300, "GR", "GRC", nullptr},
        {"Greenland", 304, "GL", "GRL", nullptr},
        {"Grenada", 308, "GD", "GRD", nullptr},
        {"Guadeloupe", 312, "GP", "GLP", nullptr},
        {"Guam", 316, "GU", "GUM", nullptr},
        {"Guatemala", 320, "GT", "GTM", nullptr},
        {"Guernsey", 831, "GG", "GGY", nullptr},
        {"Guinea", 324, "GN", "GIN", nullptr},
        {"Guinea-Bissau", 624, "GW", "GNB", nullptr},
        {"Guyana", 328, "GY", "GUY", nullptr},
        {"Haiti", 332, "HT", "HTI", nullptr},
        {"Heard Island and Mcdonald Islands", 334, "HM", "HMD", nullptr},
        {"Holy See (Vatican City State)", 336, "VA", "VAT", nullptr},
        {"Honduras", 340, "HN", "HND", nullptr},
        {"Hungary", 348, "HU", "HUN", nullptr},
        {"Iceland", 352, "IS", "ISL", nullptr},
        {"India", 356, "IN", "IND", nullptr},
        {"Indonesia", 360, "ID", "IDN", nullptr},
        {"Iran", 364, "IR", "IRN", nullptr},
        {"Iraq", 368, "IQ", "IRQ", nullptr},
        {"Ireland", 372, "IE", "IRL", nullptr},
        {"Isle of Man", 833, "IM", "IMN", nullptr},
        {"Israel", 376, "IL", "ISR", nullptr},
        {"Italy", 380, "IT", "ITA", nullptr},
        {"Jamaica", 388, "JM", "JAM", nullptr},
        {"Japan", 392, "JP", "JPN", nullptr},
        {"Jersey", 832, "JE", "JEY", nullptr},
        {"Jordan", 400, "JO", "JOR", nullptr},
        {"Kazakhstan", 398, "KZ", "KAZ", nullptr},
        {"Kenya", 404, "KE", "KEN", nullptr},
        {"Kiribati", 296, "KI", "KIR", nullptr},
        {"Korea, Democratic People's Republic of", 408, "KP", "PRK", nullptr},
        {"Korea, Republic of", 410, "KR", "KOR", nullptr},
        {"Kuwait", 414, "KW", "KWT", nullptr},
        {"Kyrgyzstan", 417, "KG", "KGZ", nullptr},
        {"Lao PDR", 418, "LA", "LAO", nullptr},
        {"Latvia", 428, "LV", "LVA", nullptr},
        {"Lebanon", 422, "LB", "LBN", nullptr},
        {"Lesotho", 426, "LS", "LSO", nullptr},
        {"Liberia", 430, "LR", "LBR", nullptr},
        {"Libya", 434, "LY", "LBY", nullptr},
        {"Liechtenstein", 438, "LI", "LIE", nullptr},
        {"Lithuania", 440, "LT", "LTU", nullptr},
        {"Luxembourg", 442, "LU", "LUX", nullptr},
        {"Macedonia, Republic of", 807, "MK", "MKD", nullptr},
        {"Madagascar", 450, "MG", "MDG", nullptr},
        {"Malawi", 454, "MW", "MWI", nullptr},
        {"Malaysia", 458, "MY", "MYS", nullptr},
        {"Maldives", 462, "MV", "MDV", nullptr},
        {"Mali", 466, "ML", "MLI", nullptr},
        {"Malta", 470, "MT", "MLT", nullptr},
        {"Marshall Islands", 584, "MH", "MHL", nullptr},
        {"Martinique", 474, "MQ", "MTQ", nullptr},
        {"Mauritania", 478, "MR", "MRT", nullptr},
        {"Mauritius", 480, "MU", "MUS", nullptr},
        {"Mayotte", 175, "YT", "MYT", nullptr},
        {"Mexico", 484, "MX", "MEX", nullptr},
        {"Micronesia", 583, "FM", "FSM", nullptr},
        {"Moldova", 498, "MD", "MDA", nullptr},
        {"Monaco", 492, "MC", "MCO", nullptr},
        {"Mongolia", 496, "MN", "MNG", nullptr},
        {"Montenegro", 499, "ME", "MNE", nullptr},
        {"Montserrat", 500, "MS", "MSR", nullptr},
        {"Morocco", 504, "MA", "MAR", nullptr},
        {"Mozambique", 508, "MZ", "MOZ", nullptr},
        {"Myanmar", 104, "MM", "MMR", nullptr},
        {"Namibia", 516, "NA", "NAM", nullptr},
        {"Nauru", 520, "NR", "NRU", nullptr},
        {"Nepal", 524, "NP", "NPL", nullptr},
        {"Netherlands", 528, "NL", "NLD", nullptr},
        {"Netherlands Antilles", 530, "AN", "ANT", nullptr},
        {"New Caledonia", 540, "NC", "NCL", nullptr},
        {"New Zealand", 554, "NZ", "NZL", nullptr},
        {"Nicaragua", 558, "NI", "NIC", nullptr},
        {"Niger", 562, "NE", "NER", nullptr},
        {"Nigeria", 566, "NG", "NGA", nullptr},
        {"Niue", 570, "NU", "NIU", nullptr},
        {"Norfolk Island", 574, "NF", "NFK", nullptr},
        {"Northern Mariana Islands", 580, "MP", "MNP", nullptr},
        {"Norway", 578, "NO", "NOR", nullptr},
        {"Oman", 512, "OM", "OMN", nullptr},
        {"Pakistan", 586, "PK", "PAK", nullptr},
        {"Palau", 585, "PW", "PLW", nullptr},
        {"Palestinian Territory, Occupied", 275, "PS", "PSE", nullptr},
        {"Panama", 591, "PA", "PAN", nullptr},
        {"Papua New Guinea", 598, "PG", "PNG", nullptr},
        {"Paraguay", 600, "PY", "PRY", nullptr},
        {"Peru", 604, "PE", "PER", nullptr},
        {"Philippines", 608, "PH", "PHL", nullptr},
        {"Pitcairn", 612, "PN", "PCN", nullptr},
        {"Poland", 616, "PL", "POL", nullptr},
        {"Portugal", 620, "PT", "PRT", nullptr},
        {"Puerto Rico", 630, "PR", "PRI", nullptr},
        {"Qatar", 634, "QA", "QAT", nullptr},
        {"Reunion", 638, "RE", "REU", nullptr},
        {"Romania", 642, "RO", "ROU", nullptr},
        {"Russian Federation", 643, "RU", "RUS", "Russia"},
        {"Rwanda", 646, "RW", "RWA", nullptr},
        {"Saint-Barthélemy", 652, "BL", "BLM", nullptr},
        {"Saint Helena", 654, "SH", "SHN", nullptr},
        {"Saint Kitts and Nevis", 659, "KN", "KNA", nullptr},
        {"Saint Lucia", 662, "LC", "LCA", nullptr},
        {"Saint-Martin (French part)", 663, "MF", "MAF", nullptr},
        {"Saint Pierre and Miquelon", 666, "PM", "SPM", nullptr},
        {"Saint Vincent and Grenadines", 670, "VC", "VCT", nullptr},
        {"Samoa", 882, "WS", "WSM", nullptr},
        {"San Marino", 674, "SM", "SMR", nullptr},
        {"Sao Tome and Principe", 678, "ST", "STP", nullptr},
        {"Saudi Arabia", 682, "SA", "SAU", nullptr},
        {"Senegal", 686, "SN", "SEN", nullptr},
        {"Serbia", 688, "RS", "SRB", nullptr},
        {"Seychelles", 690, "SC", "SYC", nullptr},
        {"Sierra Leone", 694, "SL", "SLE", nullptr},
        {"Singapore", 702, "SG", "SGP", nullptr},
        {"Slovakia", 703, "SK", "SVK", nullptr},
        {"Slovenia", 705, "SI", "SVN", nullptr},
        {"Solomon Islands", 90, "SB", "SLB", nullptr},
        {"Somalia", 706, "SO", "SOM", nullptr},
        {"South Africa", 710, "ZA", "ZAF", nullptr},
        {"South Georgia and the South Sandwich Islands", 239, "GS", "SGS", nullptr},
        {"South Sudan", 728, "SS", "SSD", nullptr},
        {"Spain", 724, "ES", "ESP", nullptr},
        {"Sri Lanka", 144, "LK", "LKA", nullptr},
        {"Sudan", 736, "SD", "SDN", nullptr},
        {"Suriname", 740, "SR", "SUR", nullptr},
        {"Svalbard and Jan Mayen Islands", 744, "SJ", "SJM", nullptr},
        {"Swaziland", 748, "SZ", "SWZ", nullptr},
        {"Sweden", 752, "SE", "SWE", nullptr},
        {"Switzerland", 756, "CH", "CHE", nullptr},
        {"Syrian Arab Republic", 760, "SY", "SYR", "Syria"},
        {"Taiwan, Republic of China", 158, "TW", "TWN", nullptr},
        {"Tajikistan", 762, "TJ", "TJK", nullptr},
        {"Tanzania", 834, "TZ", "TZA", nullptr},
        {"Thailand", 764, "TH", "THA", nullptr},
        {"Timor-Leste", 626, "TL", "TLS", nullptr},
        {"Togo", 768, "TG", "TGO", nullptr},
        {"Tokelau", 772, "TK", "TKL", nullptr},
        {"Tonga", 776, "TO", "TON", nullptr},
        {"Trinidad and Tobago", 780, "TT", "TTO", nullptr},
        {"Tunisia", 788, "TN", "TUN", nullptr},
        {"Turkey", 792, "TR", "TUR", nullptr},
        {"Turkmenistan", 795, "TM", "TKM", nullptr},
        {"Turks and Caicos Islands", 796, "TC", "TCA", nullptr},
        {"Tuvalu", 798, "TV", "TUV", nullptr},
        {"Uganda", 800, "UG", "UGA", nullptr},
        {"Ukraine", 804, "UA", "UKR", nullptr},
        {"United Arab Emirates", 784, "AE", "ARE", nullptr},
        {"United Kingdom", 826, "GB", "GBR", nullptr},
        {"United States of America", 840, "US", "USA", "EN"},
        {"United States Minor Outlying Islands", 581, "UM", "UMI", nullptr},
        {"Uruguay", 858, "UY", "URY", nullptr},
        {"Uzbekistan", 860, "UZ", "UZB", nullptr},
        {"Vanuatu", 548, "VU", "VUT", nullptr},
        {"Venezuela", 862, "VE", "VEN", nullptr},
        {"Viet Nam", 704, "VN", "VNM", nullptr},
        {"Virgin Islands, US", 850, "VI", "VIR", nullptr},
        {"Wallis and Futuna Islands", 876, "WF", "WLF", nullptr},
        {"Western Sahara", 732, "EH", "ESH", nullptr},
        {"Yemen", 887, "YE", "YEM", nullptr},
        {"Zambia", 894, "ZM", "ZMB", nullptr},
        {"Zimbabwe", 716, "ZW", "ZWE", nullptr}};

    class TRegionsMap {
        typedef THashMap<TStringBuf, TStringBuf, TCIHash<TStringBuf>, TCIEqualTo<TStringBuf>> TCodesHash;
        typedef THashMap<TStringBuf, ui16, TCIHash<TStringBuf>, TCIEqualTo<TStringBuf>> TCodesNumHash;

    private:
        void AddName(const TStringBuf& name, const TStringBuf& region) {
            if (CodesHash.find(name) != CodesHash.end()) {
                Y_ASSERT(CodesHash.find(name)->second == region);
                return;
            }

            CodesHash[name] = region;
        }

        void AddName(const TStringBuf& name, ui16 numRegion) {
            if (CodesNumHash.find(name) != CodesNumHash.end()) {
                Y_ASSERT(CodesNumHash.find(name)->second == numRegion);
                return;
            }

            CodesNumHash[name] = numRegion;
        }

        void AddSynonyms(const char* syn, const TStringBuf& region) {
            static const char* del = " ,;";
            if (!syn)
                return;
            while (*syn) {
                size_t len = strcspn(syn, del);
                AddName(TStringBuf(syn, len), region);
                syn += len;
                while (*syn && strchr(del, *syn))
                    ++syn;
            }
        }

        void AddSynonyms(const char* syn, ui16 numRegion) {
            static const char* del = " ,;";
            if (!syn)
                return;
            while (*syn) {
                size_t len = strcspn(syn, del);
                AddName(TStringBuf(syn, len), numRegion);
                syn += len;
                while (*syn && strchr(del, *syn))
                    ++syn;
            }
        }

    public:
        TRegionsMap() {
            for (size_t i = 0; i != Y_ARRAY_SIZE(RegionsInfo); ++i) {
                const TRegionInfo& val = RegionsInfo[i];

                TStringBuf region = val.Latin3Code;
                ui16 numCode = val.NumCode;

                AddName(val.Latin2Code, region);
                AddName(val.Latin3Code, region);
                AddName(val.CountryName, region);
                AddSynonyms(val.Synonyms, region);

                AddName(val.Latin2Code, numCode);
                AddName(val.Latin3Code, numCode);
                AddName(val.CountryName, numCode);
                AddSynonyms(val.Synonyms, numCode);

                if (NumCodesHash.find(numCode) != NumCodesHash.end()) {
                    Y_ASSERT(NumCodesHash.find(numCode)->second == region);
                } else {
                    NumCodesHash[numCode] = region;
                }
            }
        }

        TStringBuf RegionByCode(const TStringBuf& name) const {
            if (!name)
                return UNKNOWN_REGION;

            TCodesHash::const_iterator i = CodesHash.find(name);
            if (i == CodesHash.end())
                return UNKNOWN_REGION;

            return i->second;
        }

        TStringBuf RegionByNumCode(ui16 numCode) const {
            THashMap<ui16, TStringBuf>::const_iterator i = NumCodesHash.find(numCode);
            if (i == NumCodesHash.end())
                return UNKNOWN_REGION;

            return i->second;
        }

        ui16 RegionCodeByName(const TStringBuf& name) const {
            if (!name)
                return 0;

            TCodesNumHash::const_iterator i = CodesNumHash.find(name);
            if (i == CodesNumHash.end())
                return 0;

            return i->second;
        }

    private:
        TCodesHash CodesHash;
        THashMap<ui16, TStringBuf> NumCodesHash;
        TCodesNumHash CodesNumHash;
    };

    const char* CanonizeRegion(const char* region) {
        return Singleton<TRegionsMap>()->RegionByCode(region).data();
    }

    const char* CanonizeRegion(ui16 numRegionCode) {
        return Singleton<TRegionsMap>()->RegionByNumCode(numRegionCode).data();
    }

    ui16 RegionCodeByName(const TStringBuf& name) {
        return Singleton<TRegionsMap>()->RegionCodeByName(name);
    }

    const char* UnknownRegion() {
        return UNKNOWN_REGION;
    }

}
