#pragma once

#include "printer.h"

struct IListManager {
    virtual void DumpState(TPrinter&) const {}

    virtual ~IListManager() {}
};
