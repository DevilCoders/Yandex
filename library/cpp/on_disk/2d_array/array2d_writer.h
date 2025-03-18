#pragma once

#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/system/defaults.h>
#include <util/system/fs.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>

namespace NFile2DArrayWriter {
    class TFileWriterBase {
    protected:
        // Записи структурки в файл. В случае неудачи кидается исключение
        template <class T>
        void FPutEx(THolder<TFileOutput>& file, const T& a) {
            file->Write(&a, sizeof(a));
        }
        void FPutEx(THolder<TFileOutput>& file, const char* a, size_t len) {
            file->Write(a, len);
        }
    };

}

// Интерфейс записи на диск двумерного массива в файле (см. "array2d.h")
// Предполагается, что запись выполняется в один проход, т.е заранее количество строк массива неизвестно
template <class I>
class TFile2DArrayWriterBase: public NFile2DArrayWriter::TFileWriterBase {
protected:
    // Результирующий файл, в который пишутся заголовок и смещения начал строк
    // (первая половина 2D-массива в файле)
    TString ResultFileName;
    THolder<TFileOutput> ResultFile;
    // Файл, в который пишутся данные (вторая половина 2D-массива в файле),
    // и который приписывается к ResultFile по Finish()
    TString DataFileName;
    THolder<TFileOutput> DataFile;
    I CurrentLineNumber; // Номер текущей строки (с 0)
    I CurrentOffset;     // Смещение следующего элемента (количество записанных элементов)
    bool ReallyUseI;     // Старая версия использовала исключительно 32-битные смещения вне зависимости от I

    // Сохранить файл (без этого вызова файл сохранён не будет)
    // В случае неудачи удалить временный DataFile, ошибка проглатывается
    void Finish(size_t elemSize);
    // После Finish вызывать функции записи в файл нельзя

public:
    TFile2DArrayWriterBase(const TString& filename, I offset = 0, bool reallyUseI = false);
    ~TFile2DArrayWriterBase();

    // Запись данных
    // Начать новую строку массива (после создания объекта первая строка массива уже начата)
    void NewLine();
};

template <class I, class T>
class TFile2DArrayWriter: public TFile2DArrayWriterBase<I> {
private:
    typedef TFile2DArrayWriterBase<I> TBase;

public:
    TFile2DArrayWriter(const TString& filename, I offset = 0, bool reallyUseI = false)
        : TBase(filename, offset, reallyUseI)
    {
    }

    // Записать следующий элемент в текущую строку массива
    void Write(const T& t);

    // Записать несколько элементов в текущую строку массива
    void Write(const T* t, size_t numElements);

    void Finish();
};

template <class I>
class TFile2DArrayVariableWriter: public TFile2DArrayWriterBase<I> {
private:
    typedef TFile2DArrayWriterBase<I> TBase;

    size_t ElemSize;

public:
    TFile2DArrayVariableWriter(const TString& filename, size_t elemSize, I offset = 0, bool reallyUseI = false)
        : TBase(filename, offset, reallyUseI)
        , ElemSize(elemSize)
    {
    }

    void Write(const char* t);

    void Finish();
};

// ----------------- реализация --------------------

template <class I>
TFile2DArrayWriterBase<I>::TFile2DArrayWriterBase(const TString& filename, I offset, bool reallyUseI)
    : ResultFileName(filename)
    , ResultFile(new TFileOutput(filename))
    , DataFileName(ResultFileName + ".temp") // Временный файл создаётся рядом с целевым
    , DataFile(new TFileOutput(DataFileName))
    , CurrentLineNumber(0)
    , CurrentOffset(offset)
    , ReallyUseI(reallyUseI)
{
    // Прыгаем через заголовок
    // В старой версии был <ElemSize> + <количество строк>
    // В новой <ElemSize> + <(ui32)-1> + <sizeof(I)> + <количество строк> + <конечный размер файла>
    // Число строк будем считать в CurrentLineNumber
    if (ReallyUseI) {
        char buf[sizeof(ui32) * 4 + sizeof(I) * 2];
        memset(buf, 0, sizeof(buf));
        ResultFile->Write(buf, sizeof(buf));
    } else {
        char buf[sizeof(ui32) + sizeof(ui32)];
        memset(buf, 0, sizeof(buf));
        ResultFile->Write(buf, sizeof(buf));
    }
    FPutEx(ResultFile, CurrentOffset);
    // Теперь оба файла готовы к записи
}

template <class I>
TFile2DArrayWriterBase<I>::~TFile2DArrayWriterBase() {
    // Если хотя бы один файл не закрыт, значит не было Finish и нужно удалить файлы
    if (ResultFile || DataFile) {
        ResultFile.Destroy();
        DataFile.Destroy();
        NFs::Remove(ResultFileName);
        NFs::Remove(DataFileName);
    }
}

template <class I>
void TFile2DArrayWriterBase<I>::NewLine() {
    FPutEx(ResultFile, CurrentOffset);
    CurrentLineNumber++;
}

template <class I>
void TFile2DArrayWriterBase<I>::Finish(size_t elemSize) {
    ResultFile->Finish();

    // Запишем размер элемента и количество строк в начало ResultFile
    ResultFile.Reset(new TFileOutput(TFile(ResultFileName, OpenAlways | WrOnly))); // rewriting
    FPutEx(ResultFile, (ui32)elemSize);
    if (ReallyUseI) {
        FPutEx(ResultFile, (ui32)-1); // Маркер версии, поддерживающей произвольные смещения
        FPutEx(ResultFile, (ui32)sizeof(I));
        const ui32 headerSize = sizeof(ui32) * 4 + sizeof(I) * 2;
        FPutEx(ResultFile, headerSize); // Размер заголовка
        const I lineCount = CurrentLineNumber + 1;
        FPutEx(ResultFile, lineCount);
        FPutEx(ResultFile, (I)(headerSize + lineCount * sizeof(I) +
                               elemSize * CurrentOffset)); // Общий размер заголовка и данных
    } else {
        const ui32 lineCount = CurrentLineNumber + 1;
        FPutEx(ResultFile, lineCount);
    }

    // Закрываем файлы
    ResultFile->Finish();
    ResultFile.Destroy();
    DataFile->Finish();
    DataFile.Destroy();

    // Допишем в конец ResultFile содержимое DataFile
    NFs::Cat(ResultFileName, DataFileName);

    // Удаляем временный
    NFs::Remove(DataFileName);
}

template <class I, class T>
void TFile2DArrayWriter<I, T>::Write(const T& t) {
    TBase::FPutEx(TBase::DataFile, t);
    TBase::CurrentOffset++;
}

template <class I, class T>
void TFile2DArrayWriter<I, T>::Finish() {
    TBase::Finish(sizeof(T));
}

template <class I>
void TFile2DArrayVariableWriter<I>::Write(const char* t) {
    TBase::FPutEx(TBase::DataFile, t, ElemSize);
    TBase::CurrentOffset++;
}

template <class I, class T>
void TFile2DArrayWriter<I, T>::Write(const T* t, size_t numElements) {
    TBase::FPutEx(TBase::DataFile, static_cast<const char *>(t), sizeof(T) * numElements );
    TBase::CurrentOffset+= numElements;
}

template <class I>
void TFile2DArrayVariableWriter<I>::Finish() {
    TBase::Finish(ElemSize);
}
