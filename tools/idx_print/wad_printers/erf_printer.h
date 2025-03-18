#pragma once

#include <kernel/doom/offroad_erf_wad/erf_io.h>

#include "generic_erf_printer.h"

class TErfPrinter : public TGenericErfPrinter<NDoom::TErf2Io> {
    using TBase = TGenericErfPrinter<NDoom::TErf2Io>;
public:
    template <typename... Args>
    TErfPrinter(Args&&... args) :
        TBase(std::forward<Args>(args)...)
    {
    }
    const char* Name() override {
        return "Erf";
    }
};
