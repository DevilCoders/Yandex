#pragma once

#include "action_points.h"
#include "stream_points.h"
#include "misc_points.h"
#include "simple_module.h"

template <class T>
class TVectorBufferModule: public TSimpleModule {
protected:
    TVector<T> Buffer;

private:
    size_t Position;

    TSlaveInputPoint<const T&> InputPoint;
    TSlaveOutputPoint<const T*> OutputPoint1;
    TSlave2ArgsPoint<T&, bool&> OutputPoint2;
    TSlaveOutputPoint<size_t> VectorSizeOutput;
    TSlaveActionPoint StartPoint;
    TSlaveActionPoint FinishPoint;

protected:
    TVectorBufferModule(TString moduleName = "TVectorBufferModule", bool returnPointer = true)
        : TSimpleModule(moduleName)
        , InputPoint(InputPoint.Bind(this).template To<&TVectorBufferModule::Write>("input"))
        , OutputPoint1(OutputPoint1.Bind(this).template To<&TVectorBufferModule::Read1>(returnPointer ? "output" : ""))
        , OutputPoint2(OutputPoint2.Bind(this).template To<&TVectorBufferModule::Read2>(returnPointer ? "" : "output"))
        , VectorSizeOutput(VectorSizeOutput.Bind(this).template To<&TVectorBufferModule::Size>("vector_size_output"))
        , StartPoint(StartPoint.Bind(this).template To<&TVectorBufferModule::Start>("start"))
        , FinishPoint(FinishPoint.Bind(this).template To<&TVectorBufferModule::Finish>("finish"))
    {
    }

public:
    static TCalcModuleHolder BuildModule() {
        return new TVectorBufferModule;
    }

private:
    void Start() {
        Position = 0;
    }
    void Finish() {
        Buffer.clear();
    }
    const T* Read1() {
        return (Position < Buffer.size()) ? &Buffer[Position++] : nullptr;
    }
    void Read2(T& dest, bool& res) {
        if (res = (Position < Buffer.size())) {
            dest = Buffer[Position++];
        }
    }
    void Write(const T& val) {
        Buffer.push_back(val);
    }
    size_t Size() {
        return Buffer.size();
    }
};
