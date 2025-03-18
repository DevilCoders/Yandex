#pragma once

#include <mapreduce/interface/value.h>
#include <yweb/robot/gefest/libkiwiexport/kiwi_export_actualizer.h>
#include <library/cpp/binsaver/bin_saver.h>

#include <util/generic/strbuf.h>
#include <util/string/cast.h>
#include <util/ysafeptr.h>

namespace NHebe {

    class TTsTabFormat: public NKiwiExportActualizer::ICustomFormat {
    private:
        OBJECT_METHODS(TTsTabFormat);

    public:
        TRecord Parse(const NMR::TValue& /*key*/, const NMR::TValue& value) override {
            TStringBuf s = value.AsStringBuf();
            TStringBuf ts = s.NextTok('\t');
            TRecord res;
            res.Timestamp = FromString<ui32>(ts);
            res.OutputValue = value;
            return res;
        }
        int operator&(IBinSaver& /*saver*/) override {
            return 0;
        }
    };
}
REGISTER_SAVELOAD_NM_CLASS(144322, NHebe, TTsTabFormat);
