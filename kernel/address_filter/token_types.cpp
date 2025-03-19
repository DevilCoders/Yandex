#include "token_types.h"

#include <util/generic/string.h>
#include <util/stream/str.h>

namespace NAddressFilter {

const TString TYPE_DESCRIPTIONS[] =  {
                                        TString("EMPTY"),
                                        TString("HOUSE"),
                                        TString("DESCRIPTOR"),
                                        TString("BUILDING"),
                                        TString("CITY"),
                                        TString("LINE"),
                                        TString("VO_11"),
                                        TString("VO_21"),
                                        TString("VO_22"),

                                        TString("STREET_DICT_11"),
                                        TString("STREET_DICT_21"),
                                        TString("STREET_DICT_22"),
                                        TString("STREET_DICT_31"),
                                        TString("STREET_DICT_32"),
                                        TString("STREET_DICT_33"),
                                        TString("STREET_DICT_41"),
                                        TString("STREET_DICT_42"),
                                        TString("STREET_DICT_43"),
                                        TString("STREET_DICT_44"),

                                        TString("METRO_11"),
                                        TString("METRO_21"),
                                        TString("METRO_22"),
                                        TString("METRO_31"),
                                        TString("METRO_32"),
                                        TString("METRO_33"),

                                        TString("METRO_DESCR"),
                                        TString("NAME_DESCR"),

                                        TString("CITY_DICT_11"),

                                        TString("CAPITAL"),
                                        TString("NUMBER"),
                                        TString("NUMBER_END"),
                                        TString("STREET_DICT_11S"),
                                        TString("STREET_DICT_21S"),
                                        TString("STREET_DICT_22S"),
                                        TString("STREET_DICT_31S"),
                                        TString("STREET_DICT_32S"),
                                        TString("STREET_DICT_33S"),
                                        TString("STREET_DICT_41S"),
                                        TString("STREET_DICT_42S"),
                                        TString("STREET_DICT_43S"),
                                        TString("STREET_DICT_44S"),
                                        TString("MONTH"),
                                        TString("ADDRESS"),
                                        TString("DESCRIPTOR_NN"),

                                        TString("CITY_DICT_21"),
                                        TString("CITY_DICT_22")
                                    };

const wchar16 ALLOWED_PUNKTUATION[] = {'.', ',', ':', ';', '-'};
size_t ALLOWED_PUNKTUATION_SIZE = sizeof(ALLOWED_PUNKTUATION) / sizeof(wchar16);


TString GetDebugTokenType(const TTokenType& tokenType) {

    TStringStream stream;
    for (size_t i = 0; i < tokenType.Size(); i++) {
        if (tokenType.Get(i))
            stream << TYPE_DESCRIPTIONS[i] << " ";
    }

    return stream.Str();
}

}
