#pragma once

#include "simple_module.h"

template <class TWriter, class TInputType>
class T2DArrayWriter: public TSimpleModule {
private:
    class IWriterFactory {
    public:
        virtual ~IWriterFactory() = default;
        virtual TAutoPtr<TWriter> CreateWriter() const = 0;
    };
    template <class TArg>
    class TOneArgFactory: public IWriterFactory {
    private:
        TArg Arg;

    public:
        TOneArgFactory(TArg arg)
            : Arg(arg)
        {
        }
        TAutoPtr<TWriter> CreateWriter() const override {
            return new TWriter(Arg);
        }
    };
    template <class TArg1, class TArg2>
    class TTwoArgFactory: public IWriterFactory {
    private:
        TArg1 Arg1;
        TArg2 Arg2;

    public:
        TTwoArgFactory(TArg1 arg1, TArg2 arg2)
            : Arg1(arg1)
            , Arg2(arg2)
        {
        }
        TAutoPtr<TWriter> CreateWriter() const override {
            return new TWriter(Arg1, Arg2);
        }
    };

    THolder<IWriterFactory> WriterFactory;
    THolder<TWriter> Writer;

public:
    template <class TArg>
    T2DArrayWriter(TArg arg)
        : TSimpleModule("T2DArrayWriter")
        , WriterFactory(new TOneArgFactory<TArg>(arg))
    {
        Init();
    }
    template <class TArg1, class TArg2>
    T2DArrayWriter(TArg1 arg1, TArg2 arg2)
        : TSimpleModule("T2DArrayWriter")
        , WriterFactory(new TTwoArgFactory<TArg1, TArg2>(arg1, arg2))
    {
        Init();
    }

private:
    void Init() {
        Bind(this).template To<&T2DArrayWriter::NewLine>("new_line");
        Bind(this).template To<TInputType, &T2DArrayWriter::Write>("input");
        Bind(this).template To<&T2DArrayWriter::Start>("start");
        Bind(this).template To<&T2DArrayWriter::Finish>("finish");
    }

    void Start() {
        Writer.Reset(WriterFactory->CreateWriter());
    }
    void Finish() {
        Writer->Finish();
        Writer.Destroy();
    }

    void NewLine() {
        Writer->NewLine();
    }
    void Write(TInputType val) {
        Writer->Write(val);
    }
};
