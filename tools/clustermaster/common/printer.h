#pragma once

#include <util/stream/output.h>

class TPrinter {
private:
    IOutputStream* Out;
    const ui32 Indent;
public:
    TPrinter(IOutputStream& out = Cerr, ui32 indent = 0)
        : Out(&out), Indent(indent)
    {}

    template <typename T>
    void Println(const T& value) {
        for (ui32 i = 0; i < Indent; ++i) {
            *Out << "    ";
        }
        *Out << value << Endl;
    }

    TPrinter Next() const {
        return TPrinter(*Out, Indent + 1);
    }

};
