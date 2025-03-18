#pragma once


#include "generic_reg_erf_printer.h"

#include <kernel/doom/offroad_reg_erf_wad/reg_erf_io.h>


class TRegErfPrinter : public TGenericRegErfPrinter<NDoom::TRegErfIo> {
    using TBase = TGenericRegErfPrinter<NDoom::TRegErfIo>;
public:
    template <typename... Args>
    TRegErfPrinter(Args&&... args) :
        TBase(std::forward<Args>(args)...)
    {
    }
    const char* Name() override {
        return "RegErf";
    }
};
