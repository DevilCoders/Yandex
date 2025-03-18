#pragma once

#include <util/system/compat.h>
#include <util/system/yassert.h>
#include <util/system/defaults.h>
#include <util/system/file.h>
#include <util/system/filemap.h>
#include <util/stream/output.h>
#include <util/memory/blob.h>
#include <util/memory/alloc.h>
#include <util/generic/buffer.h>
#include <util/generic/cast.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

#include <iterator>

// Интерфейс чтения двумерного массива, отображённого на диск.
// Строки массива могут содержать различное количество элементов.
// Формат файла, содержащего двумерный массив:
//     Заголовок(описан ниже)
//     sizeof(I) * <количество строк> - смещения начал строк в массиве данных
//     sizeof(T) * <общее количество элементов массива> - данные
// Формат заголовка:
//   Современный:
//     4 байта - размер T
//     4 байта - (ui32)-1 маркер новой версии
//     4 байта - размер I
//     4 байта - размер заголовка
//     sizeof(I) - количество строк
//     sizeof(I) - конечный размер файла
//   Deprecated:
//     4 байта - размер T
//     4 байта - количество строк
// I - тип, задающий смещения начал строк в массиве данных
// T - тип данных, хранимый в массиве
template <class I, class T>
class FileMapped2DArray {
protected:
    TBlob DataHolder; // Хранилище блока данных
    I* s_Ptr;         // Массив смещений начал одномерных массивов, соответствующих строкам, в массиве данных m_Ptr
    T* m_Ptr;         // Массив данных
    I s_Size;         // Размер первого измерения двумерного массива (количество строк, размер массива s_Ptr)
    T* m_Finish;      // Указатель на первый элемент за массивом данных m_Ptr
    I ElemSize;
    TVector<T> m_Copy;        // Copy of the data - used in "polite" mode, when sizeof(T) != ElemSize
    TVector<I> m_OffsetsCopy; // Copy of the offsets - used in "polite" mode, when sizeof(T) != ElemSize
    TString m_FileName;       // Used to emit a log message when data is copied in the polite mode
    static char m_Dummy[sizeof(T)];

public:
    FileMapped2DArray()
        : s_Ptr(nullptr)
        , m_Ptr(nullptr)
        , s_Size(0)
        , m_Finish(nullptr)
        , ElemSize(0)
    {
        memset(m_Dummy, 0, sizeof m_Dummy);
    }
    explicit FileMapped2DArray(const TString& name, bool polite = false)
        : DataHolder(TBlob::FromFile(name))
        , m_FileName(name)
    {
        memset(m_Dummy, 0, sizeof m_Dummy);
        if (!InitSelf(polite))
            ythrow yexception() << "Cannot init mapped 2D array from file " << name.Quote();
    }
    FileMapped2DArray(const TBlob& data, bool polite = false)
        : DataHolder(data)
    {
        memset(m_Dummy, 0, sizeof m_Dummy);
        if (!InitSelf(polite))
            ythrow yexception() << "Cannot init mapped 2D array from blob";
    }
    FileMapped2DArray(const TMemoryMap& mapping, bool polite = false)
        : DataHolder(TBlob::FromMemoryMap(mapping, 0, mapping.Length()))
        , m_FileName(mapping.GetFile().GetName())
    {
        memset(m_Dummy, 0, sizeof m_Dummy);
        if (!InitSelf(polite))
            ythrow yexception() << "Cannot init mapped 2D array from mapping " << mapping.GetFile().GetName().Quote();
    }
    ~FileMapped2DArray() {
        /// don't call Term() here as ~MappedFile also calls it's Term()
        m_Ptr = nullptr;
    }
    /// Deprecated. Use Init(const char* name) instead
    bool init(const TString& name) {
        return Init(name);
    }
    /// Loads array from file
    bool Init(const TString& name, bool polite = false) {
        m_FileName = name;
        DataHolder = TBlob::FromFile(name);
        return InitSelf(polite);
    }
    /// Loads array from file
    bool InitPrecharged(const TString& name, bool polite = false) {
        m_FileName = name;
        DataHolder = TBlob::PrechargedFromFile(name);
        return InitSelf(polite);
    }
    /// Deprecated. Use Init(const TBlob& data) instead
    bool init(const TBlob& data) {
        return Init(data);
    }
    /// Loads array from blob
    bool Init(const TBlob& data, bool polite = false) {
        DataHolder = data;
        return InitSelf(polite);
    }
    /// Loads array from file mapping
    bool Init(const TMemoryMap& mapping, bool polite = false) {
        m_FileName = mapping.GetFile().GetName();
        DataHolder = TBlob::FromMemoryMap(mapping, 0, mapping.Length());
        return InitSelf(polite);
    }
    /// Deprecated. Use Term() instead
    void term() {
        Term();
    }
    /// Frees resources
    void Term() {
        s_Size = 0;
        s_Ptr = nullptr;
        m_Ptr = nullptr;
        m_Finish = nullptr;
        DataHolder.Drop();
        TVector<T>().swap(m_Copy);
        TVector<I>().swap(m_OffsetsCopy);
    }
    const T* operator[](size_t pos) const {
        Y_ASSERT(pos < Size());
        // Для совместимости с файлами, созданными 32-битными программами:
        // Для некоторых файлов (например cmap-ов) смещения начал одномерных массивов в s_Ptr
        // зачем-то записывались декрементированными на 1
        // Так вполне могло случиться, что там записано I(-1) (всегда было s_Ptr[0] == I(-1)),
        // а т.к I - обычно ui32, то
        // m_Ptr + s_Ptr[pos1] = m_Ptr + 0xFFFFFFFF в результате переполнений на 32-битной машине
        // давало m_Ptr - 1. На 64-битной машине этого не происходит и для смещения ui32(-1)
        // получается большой указатель, не имеющий к реальным данным никакого отношения.
        const I& offset = s_Ptr[pos];
        if (offset == I(-1)) {
            return m_Ptr - 1;
        } else {
            return m_Ptr + offset;
        }
    }
    /// Returns pointer pointing immediately after the last element
    const T* VeryEnd() const {
        return m_Finish;
    }
    /// Deprecated. Use VeryEnd() instead
    const T* very_end() const {
        return VeryEnd();
    }
    size_t Size() const {
        return s_Size;
    }
    /// Deprecated. Use Size() instead
    size_t size() const {
        return Size();
    }
    const T& GetAt(size_t pos1, size_t pos2) const {
        if (pos1 >= s_Size)
            return *reinterpret_cast<T*>(m_Dummy);
        const T* const ptr = operator[](pos1) + pos2;
        if (ptr >= m_Finish)
            return *reinterpret_cast<T*>(m_Dummy);
        return *ptr;
    }

    const T* GetBegin(size_t pos) const {
        if (pos >= s_Size)
            return nullptr;
        return operator[](pos);
    }
    const T* GetEnd(size_t pos) const {
        if (pos >= s_Size)
            return nullptr;
        return pos + 1 < Size() ? operator[](pos + 1) : m_Finish;
    }
    size_t GetLength(size_t pos) const {
        if (!Size())
            return 0;

        const T* begin = operator[](pos);
        const T* end;
        if (pos + 1 < Size())
            end = operator[](pos + 1);
        else
            end = (s_Ptr[0] == I(-1)) ? m_Finish - 1 : m_Finish;
        return end - begin;
    }

private:
    bool InitSelf(bool polite) {
        const void* data = DataHolder.Data();
        const size_t holderSize = DataHolder.Size();
        Y_VERIFY(data && data != (void*)(-1));
        ElemSize = *(ui32*)data;
        // or we will crash with DIVBYZERO a bit later
        Y_ENSURE_EX(ElemSize, yexception() << "Invalid data: FileMapped2DArray array element size cannot be zero. ");
        ui32 rowsNumber = *((ui32*)data + 1);
        bool reallyUseI = rowsNumber == (ui32)-1;
        I fileSize = 0;
        if (reallyUseI) {
            // TODO: check s_Size on negative values (in case typeof(I) == int)
            s_Size = *(I*)((char*)data + 4 * sizeof(ui32));
            s_Ptr = (I*)((char*)data + 4 * sizeof(ui32) + 2 * sizeof(I));
            fileSize = *(I*)((char*)data + 4 * sizeof(ui32) + sizeof(I));
        } else {
            // TODO: check s_Size on negative values (in case typeof(I) == int)
            s_Size = rowsNumber;
            s_Ptr = (I*)((ui32*)data + 2);
        }

        if (ElemSize != sizeof(T)) {
            if (!polite)
                return false;
            char* begin = (char*)(s_Ptr + s_Size);
            char* end = (char*)data + holderSize;
            if ((end - begin) % ElemSize)
                return false;
            size_t size = (end - begin) / ElemSize;
            m_Copy.resize(size);
            size_t copySize = Min<size_t>(ElemSize, sizeof(T));
            for (size_t i = 0; i < size; ++i)
                memcpy(&m_Copy[i], begin + i * ElemSize, copySize);
            m_Ptr = m_Copy.begin();
            m_Finish = m_Copy.end();
            m_OffsetsCopy.resize(s_Size);
            std::copy(s_Ptr, s_Ptr + s_Size, m_OffsetsCopy.begin());
            s_Ptr = m_OffsetsCopy.begin();
            DataHolder.Drop();
            Cerr << "WARNING: FileMapped2DArray was copied in the polite mode; created from "
                 << (m_FileName.empty() ? "(blob)" : m_FileName)
                 << "; on disk record size: " << ElemSize << "; process record size: " << sizeof(T)
                 << "; data size (memory wasted): " << (end - begin) << " bytes." << Endl;
        } else {
            m_Ptr = (T*)(s_Ptr + s_Size);
            m_Finish = (T*)((char*)data + holderSize);
        }

        if (((char*)m_Finish - (char*)m_Ptr) % sizeof(T) || reallyUseI && holderSize != fileSize) {
            Term();
            return false;
        }
        return true;
    }
};

template <class I, class T>
char FileMapped2DArray<I, T>::m_Dummy[sizeof(T)];

template <class I, class T>
class FileMapped2DArrayWritable: public TMemoryMap, public FileMapped2DArray<I, T> {
private:
    using TBase = FileMapped2DArray<I, T>;

    T* GetMutableAt(size_t pos) {
        if (pos >= TBase::Size())
            return nullptr;
        const I& offset = TBase::s_Ptr[pos];
        if (offset == I(-1)) {
            return TBase::m_Ptr - 1;
        } else {
            return TBase::m_Ptr + offset;
        }
    }

public:
    explicit FileMapped2DArrayWritable(const TString& name, bool polite = false)
        : TMemoryMap(name, TMemoryMapCommon::oRdWr)
        , TBase(*this, polite)
    {
    }

    bool SetAt(size_t pos, const T& value) {
        T* mutableValue = GetMutableAt(pos);
        if (!mutableValue)
            return false;
        if (TBase::GetLength(pos) != 1)
            return false;
        *mutableValue = value;
        return true;
    }
};

//
// Copied verbatim from util/draft/array2d_ext.h
//
template <class I, class T>
class TFileIterated2DArrayExt {
private:
    class TBufferWrapper {
    private:
        TBuffer Buffer;
        size_t Position;
        size_t Used;
        size_t MaxMemSize;
        size_t PrevElementSize;

    public:
        TBufferWrapper(size_t memSize)
            : Position(0)
            , Used(0)
            , MaxMemSize(memSize)
            , PrevElementSize(0)
        {
        }

        size_t FillFromFile(TFile& inFile) {
            Buffer.ChopHead(Position);
            size_t choppedSize = Buffer.size();

            Buffer.Resize(MaxMemSize);
            size_t sizeForRead = MaxMemSize - choppedSize;
            size_t readed = inFile.Read(Buffer.data() + choppedSize, sizeForRead);
            Used = readed + choppedSize;

            Buffer.Resize(Used);
            Position = 0;
            return readed;
        }

        template <typename T2>
        const T2* Next() {
            return Next<T2>(sizeof(T2));
        }

        template <typename T2>
        const T2* Next(size_t elemSize) {
            Position = GetNewPosition(PrevElementSize);
            PrevElementSize = 0;
            const T2* elem = Current<T2>();
            if (elem)
                PrevElementSize = elemSize;
            return elem;
        }

        template <typename T2>
        const T2* Current() {
            if (!CheckElement(sizeof(T2)))
                return nullptr;
            return reinterpret_cast<const T2*>(Buffer.data() + Position);
        }

    private:
        size_t GetNewPosition(size_t s) const {
            Y_VERIFY(CheckElement(s), "Wrong postion in buffer");
            return Position + s;
        }

        bool CheckElement(size_t s) const {
            return CheckElement(Position, s);
        }

        bool CheckElement(size_t pos, size_t s) const {
            return (pos + s) <= Used;
        }
    };

public:
    struct TElement {
        size_t Id;
        const T* Value;
        TElement()
            : Id(0)
            , Value(nullptr)
        {
        }
    };

private:
    typedef TVector<I> TOffsets;
    typedef typename TOffsets::const_iterator TOffsetCIterator;

private:
    TBufferWrapper Buffer;
    TOffsets Offsets;
    size_t ElemSize;
    size_t FilePos;
    size_t FileSize;
    THolder<TFile> InFile;
    TElement Element;
    size_t ElementsCount;
    size_t ElementsReaded;
    TOffsetCIterator CurrentOffsetIt;

public:
    explicit TFileIterated2DArrayExt(size_t memSizeMB)
        : Buffer(memSizeMB << 20)
    {
        ResetCounters();
    }

    explicit TFileIterated2DArrayExt(const TString& name, size_t memSizeMB, size_t defaultElemSize = sizeof(T))
        : Buffer(memSizeMB << 20)
        , ElementsReaded(0)
    {
        Init(name, defaultElemSize);
    }

    const TElement* Current() {
        if (Element.Value == nullptr)
            return nullptr;
        return &Element;
    }

    const TElement* Next() {
        Element.Value = Buffer.template Next<T>(ElemSize);
        if (Element.Value == nullptr) {
            LoadNextPortion();
            Element.Value = Buffer.template Next<T>(ElemSize);
        }

        if (Element.Value != nullptr) {
            ++ElementsReaded;
            if (ElementsReaded > ElementsCount) {
                ythrow yexception() << "Error!!! Readed: " << ElementsReaded
                                    << " data size: " << ElementsCount;
            }
            Element.Id = GetCurrentId();
        }

        return Current();
    }

    size_t Size() const {
        return +Offsets;
    }

private:
    void ResetCounters() {
        ElemSize = 0;
        FilePos = 0;
        FileSize = 0;
        ElementsCount = 0;
        ElementsReaded = 0;
    }

    void Init(const TString& name, size_t defaultElemSize = sizeof(T)) {
        ResetCounters();
        InFile.Reset(new TFile(name, RdOnly));
        TFile& inFile = *InFile.Get();
        FileSize = inFile.GetLength();
        FilePos = Buffer.FillFromFile(inFile);
        ElemSize = *Buffer.template Next<ui32>();
        if (!ElemSize)
            ElemSize = defaultElemSize;
        ui32 offsetsCount = *Buffer.template Next<ui32>();
        Offsets.resize(offsetsCount);
        ValidateFileSize();
        LoadOffsets();
    }

    bool LoadNextPortion() {
        size_t readed = Buffer.FillFromFile(*InFile);
        if (readed == 0)
            return false;
        FilePos += readed;
        return true;
    }

    void ValidateFileSize() {
        // ElemSize + OffsetsCount
        size_t fSize = 2 * sizeof(ui32);
        fSize += sizeof(I) * Offsets.size();
        size_t dataSize = FileSize - fSize;
        ElementsCount = dataSize / ElemSize;
        if ((dataSize % ElemSize) != 0) {
            ythrow yexception() << "Cannot init extendable mapped 2D array: bad data size " << dataSize << ", must be multiple of element size " << ElemSize;
        }
    }

    void LoadOffsets() {
        for (ui32 i = 0; i != Offsets.size(); ++i) {
            const I* o = Buffer.template Next<I>();
            if (!o) {
                if (LoadNextPortion())
                    o = Buffer.template Next<I>();
                else
                    ythrow yexception() << "Unexpected end of file";
            }
            Y_VERIFY(o != nullptr, "Unexpected end of file");
            Offsets[i] = *o;
        }
        CurrentOffsetIt = Offsets.begin();
    }

    size_t GetCurrentId() {
        size_t elemetId = ElementsReaded ? ElementsReaded - 1 : 0;
        TOffsetCIterator it = CurrentOffsetIt;
        for (; it != Offsets.end(); ++it) {
            if (*it > elemetId)
                break;
        }
        Y_VERIFY(it != Offsets.begin(), "Unexpected end of offsets");
        CurrentOffsetIt = --it;
        return CurrentOffsetIt - Offsets.begin();
    }
};
