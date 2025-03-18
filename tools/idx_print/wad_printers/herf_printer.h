#pragma once

#include <kernel/doom/offroad_herf_wad/herf_io.h>

#include "generic_erf_printer.h"

class THostErfPrinter : public TGenericErfPrinter<NDoom::THostErfIo> {
    using TBase = TGenericErfPrinter<NDoom::THostErfIo>;
public:
    template <typename... Args>
    THostErfPrinter(Args&&... args) :
        TBase(std::forward<Args>(args)...)
    {
    }
    const char* Name() override {
        return "HostErf";
    }
};
