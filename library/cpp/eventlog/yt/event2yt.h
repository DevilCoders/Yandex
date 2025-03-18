#pragma once

#include <library/cpp/eventlog/eventlog.h>
#include <mapreduce/yt/interface/node.h>

NYT::TNode EventToYtNode(const TEvent& event);
