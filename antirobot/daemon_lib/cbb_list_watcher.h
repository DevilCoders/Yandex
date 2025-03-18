#pragma once

#include "cbb.h"
#include "cbb_list_manager.h"

#include <library/cpp/threading/future/future.h>

#include <util/generic/vector.h>


namespace NAntiRobot {


class TCbbListWatcher {
public:
    void Add(ICbbListManager* manager) {
        Managers.push_back(manager);
    }

    void Poll(ICbbIO* cbb);

public:
    TVector<ICbbListManager*> Managers;
    NThreading::TFuture<void> Future = NThreading::MakeFuture();
};


} // namespace NAntiRobot
