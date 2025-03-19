#include "grouping.h"

const TString GetGroupingDescription() {
    return TString(
        "Grouping field contents (see also https://nda.ya.ru/3QbR5u):\n"
        "  print-group-index  [was: group]:  "
            "Group by field contents, print different integer index for each group\n"
        "  print-result-index [was: none]:   "
            "Print surrogate integer for each result (unique within one request)\n"
        "  print-as-is        [was: domain]: "
            "Print fields contents AS IS (typically, print domain)"
    );
}
