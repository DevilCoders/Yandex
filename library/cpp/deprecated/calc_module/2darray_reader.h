#pragma once

#include "simple_module.h"

template <class TReader>
class T2DArrayReader: public TSimpleModule {
public:
    typedef typename TReader::TIndex TIndex;
    typedef typename TReader::TValue TValue;

    class IReaderFactory {
    public:
        virtual ~IReaderFactory() = default;
        virtual TAutoPtr<TReader> CreateReader() const = 0;
    };
    template <class TArg>
    class TOneArgFactory: public IReaderFactory {
    private:
        TArg Arg;

    public:
        TOneArgFactory(TArg arg)
            : Arg(arg)
        {
        }

        virtual TAutoPtr<TReader> CreateReader() const {
            return new TReader(Arg);
        }
    };
    template <class TArg1, class TArg2>
    class TTwoArgFactory: public IReaderFactory {
    private:
        TArg1 Arg1;
        TArg2 Arg2;

    public:
        TTwoArgFactory(TArg1 arg1, TArg2 arg2)
            : Arg1(arg1)
            , Arg2(arg2)
        {
        }

        TAutoPtr<TReader> CreateReader() const override {
            return new TReader(Arg1, Arg2);
        }
    };

    THolder<IReaderFactory> ReaderFactory;
    THolder<TReader> Reader;

public:
    template <class TArg>
    T2DArrayReader(TArg arg)
        : TSimpleModule("T2DArrayReader")
        , ReaderFactory(new TOneArgFactory<TArg>(arg))
    {
        Init();
    }

    template <class TArg1, class TArg2>
    T2DArrayReader(TArg1 arg1, TArg2 arg2)
        : TSimpleModule("T2DArrayReader")
        , ReaderFactory(new TTwoArgFactory<TArg1, TArg2>(arg1, arg2))
    {
        Init();
    }

private:
    void Init() {
        Bind(this).template To<size_t, &T2DArrayReader::GetSize>("size_output");
        Bind(this).template To<size_t, size_t&, &T2DArrayReader::GetLength>("length_output");
        Bind(this).template To<size_t, size_t, const TValue*&, &T2DArrayReader::GetData>("data_output");
        Bind(this).template To<&T2DArrayReader::Start>("start");
        Bind(this).template To<&T2DArrayReader::Finish>("finish");
    }

    void Start() {
        Reader.Reset(ReaderFactory->CreateReader());
    }

    void Finish() {
        Reader.Destroy();
    }

    size_t GetSize() {
        return Reader->Size();
    }

    void GetLength(size_t pos, size_t& length) {
        length = Reader->GetLength(pos);
    }

    void GetData(size_t i, size_t j, const TValue*& res) {
        res = &Reader->GetAt(i, j);
    }
};
