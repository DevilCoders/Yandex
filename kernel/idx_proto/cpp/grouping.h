#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/map.h>
#include <util/generic/serialized_enum.h>
#include <util/string/cast.h>

// See also FML-474 about alternate namings
enum EDomainGrouping {
    GT_NONE /* "print-result-index", "none",  */,
    GT_GROUP /* "print-group-index", "group" */,
    GT_PRINT_DOMAIN /* "print-as-is", "domain" */
};


const TString GetGroupingDescription();

struct TDomainGrouper {
    TDomainGrouper(const EDomainGrouping grouping)
        : Grouping(grouping)
    {}

    /// Obtain group name
    TString CalculateGroup(const TString& domain, const size_t resultIndex = Max<size_t>()) {
        // deciding what to print in grouping column
        switch (Grouping) {
        case GT_PRINT_DOMAIN:
            return domain;
        case GT_GROUP:
            if (!Groups.FindPtr(domain))
                Groups[domain] = Groups.size();
            return ToString(Groups[domain]);
        case GT_NONE:
        default:
            Y_ENSURE(resultIndex != Max<size_t>(), "GT_NONE mode is disabled for this domain grouper. ");
            return ToString(resultIndex);
        }
    }

    /// Reset groups (used on new request processing started)
    void Reset() {
        Groups.clear();
    }

    /// domain groups info
    typedef TMap<TString, size_t> TDomainGroups;

    TDomainGroups Groups;
    EDomainGrouping Grouping;
};
