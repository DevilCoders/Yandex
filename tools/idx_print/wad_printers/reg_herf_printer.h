#pragma once

#include <kernel/doom/offroad_reg_herf_wad/reg_herf_io.h>

#include "generic_reg_erf_printer.h"

class TRegHostErfPrinter : public TGenericRegErfPrinter<NDoom::TRegHostErfIo> {
    using TBase = TGenericRegErfPrinter<NDoom::TRegHostErfIo>;
public:
    template <typename... Args>
    TRegHostErfPrinter(Args&&... args) :
        TBase(std::forward<Args>(args)...)
    {
    }
    const char* Name() override {
        return "RegHostErf";
    }
};
