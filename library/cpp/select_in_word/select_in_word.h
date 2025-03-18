#pragma once

#include <util/system/types.h>

ui64 SelectInWord(ui64 x, int k) noexcept;
ui64 SelectInWordX86(ui64 x, int k) noexcept;
ui64 SelectInWordBmi2(ui64 x, int k) noexcept;
