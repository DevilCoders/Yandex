#pragma once

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/library/storage/query/request.h>
#include <kernel/common_server/util/cgi_processing.h>
#include <kernel/common_server/library/storage/selection/selection.h>

template <class TSelectObjectPolicy, class TDBObject>
class THistoryEventsFilterImpl : public NCS::NSelection::TSelection<typename TDBObject::TFilter, typename TDBObject::TSorting> {
    using TBase = NCS::NSelection::TSelection<typename TDBObject::TFilter, typename TDBObject::TSorting>;
};
