#pragma once

#include <util/system/file.h>
#include <util/stream/output.h>
#include <util/ysaveload.h>
#include <util/generic/noncopyable.h>

class TSysFileRequestSerializer: public IInputStream
                               , public IOutputStream {
    public:
        TSysFileRequestSerializer(TFile& f);
        TSysFileRequestSerializer(const char* fName, bool readOnly);
        ~TSysFileRequestSerializer() override;

    private:
        size_t DoRead(void* ptr, size_t len) override;
        void DoWrite(const void* ptr, size_t len) override;
    private:
        TFile F;
};
