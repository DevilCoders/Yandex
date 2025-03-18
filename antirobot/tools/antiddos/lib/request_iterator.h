#pragma once

#include <antirobot/daemon_lib/fullreq_info.h>

#include <util/generic/string.h>

struct IRequestIterator
{
    virtual ~IRequestIterator() = default;
    virtual THolder<NAntiRobot::TRequest> Next() = 0;
};

TAutoPtr<IRequestIterator> CreateTextLogIterator(const TString& fileName);
TAutoPtr<IRequestIterator> CreateEventLogIterator(const TString& fileName);
