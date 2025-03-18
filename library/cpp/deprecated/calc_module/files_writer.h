#pragma once

#include <util/stream/file.h>
#include <util/generic/vector.h>
#include <util/system/sys_alloc.h>

#include "action_points.h"
#include "misc_points.h"
#include "simple_module.h"

class TFilesWriter: public TSimpleModule {
private:
    TOFStream* Files;
    ui32 FilesNum;

    TSlave3ArgsPoint<ui32, const void*, size_t> WritePoint;
    TMasterCopyPoint<ui32> FilesNumHolder;
    TMasterAnswerPoint<ui32, TString> FileNamesGenerator;
    TSlaveActionPoint StartPoint;
    TSlaveActionPoint FinishPoint;

    TFilesWriter()
        : TSimpleModule("TFilesWriter")
        , Files(nullptr)
        , FilesNum(0)
        , WritePoint(WritePoint.Bind(this).To<&TFilesWriter::Write>("input"))
        , FilesNumHolder(this, 0, "filesnum_input")
        , FileNamesGenerator(this, "filename_input")
        , StartPoint(StartPoint.Bind(this).To<&TFilesWriter::Start>("start"))
        , FinishPoint(FinishPoint.Bind(this).To<&TFilesWriter::Finish>("finish"))
    {
    }
    ~TFilesWriter() override {
        Finish();
    }

public:
    static TCalcModuleHolder BuildModule() {
        return new TFilesWriter;
    }

private:
    void Start() {
        FilesNum = 0;
        ui32 filesNum = FilesNumHolder.GetValue();
        Files = (TOFStream*)y_allocate(filesNum * sizeof(TOFStream));
        for (ui32 i = 0; i < filesNum; ++i, ++FilesNum)
            new (&Files[i]) TOFStream(FileNamesGenerator.Answer(i));
    }
    void Write(ui32 fileNum, const void* buf, size_t len) {
        Files[fileNum].Write(buf, len);
    }
    void Finish() {
        for (ui32 i = 0; i < FilesNum; i++) {
            Files[i].Flush();
            Files[i].~TOFStream();
        }
        y_deallocate(Files);
        Files = nullptr;
        FilesNum = 0;
    }
};
