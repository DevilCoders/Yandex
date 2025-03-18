#pragma once

#include <util/generic/ptr.h>
#include <util/stream/input.h>
#include <util/stream/output.h>

/// Ввод бинарных данных порциями.
/// @details Аналогично TChunkedInput, но для бинарных данных.
class TBinaryChunkedInput: public IInputStream {
public:
    TBinaryChunkedInput(IInputStream* input);
    ~TBinaryChunkedInput() override;

private:
    size_t DoRead(void* buf, size_t len) override;

private:
    class TImpl;
    THolder<TImpl> Impl_;
};

/// Вывод бинарных данных порциями.
/// @details Аналогично TChunkedOutput, но для бинарных данных.
class TBinaryChunkedOutput: public IOutputStream {
public:
    TBinaryChunkedOutput(IOutputStream* output);
    ~TBinaryChunkedOutput() override;

private:
    void DoWrite(const void* buf_in, size_t len) override;
    void DoFinish() override;

private:
    class TImpl;
    THolder<TImpl> Impl_;
};
