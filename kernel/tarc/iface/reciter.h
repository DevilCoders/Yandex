#pragma once

#include <errno.h>
#include <util/system/defaults.h>
#include <util/generic/buffer.h>
#include <util/stream/file.h>

/**
Итератор по файлу с записями переменной длины, использует чтение записей в большой буффер.
Записи начинаются с POD-заголовка TEntryHeader, TEntryHeader::SizeOf() возвращает длину всей записи,
включая длину sizeof(TEntryHeader) собственно заголовка.

RefillBuffer() заполняет буфер, возвращает false, если файл закончился.
Ошибки ввода сигнализируются exception'ами.

Next() возвращает указатель на очередную запись в виде ее заголовка, или NULL, если достигнут конец буфера.
Гарантируется, что память, возвращенная Next(), включает запись целиком.
Все указатели, которые вернула Next(), валидны до следующего вызова RefillBuffer().

NextAuto() автоматически вызывает RefillBuffer() в случае, если достигнут конец буфера.
Таким образом, она возвращает NULL только в случае, если файл закончился, однако
указатели на предыдущие записи, возвращенные этой функцией, в общем случае не валидны.

Память под буфер выделяется один раз в конструкторе, однако может быть реаллоцирована при чтении файла,
если в буфер не влезает одна запись. Однако файлы можно открывать/закрывать, повторно используя
выделенный буфер.
*/

#define FS_BLOCK_SIZE 512

template<class TEntryHeader>
class TFileRecordIterator : TNonCopyable {
private:
    TFile File;     // File to read from
    TBuffer Buffer; // Buffer with data read
    char* CurPos;   // Current position in Buffer
    bool Eof;

    //! returns a valid pointer if the record at the specified offset is within the buffer
    char* CheckRecord(i64 offset) {
        const i64 filePosition = File.GetPosition();
        Y_VERIFY(filePosition >= 0, "invalid position value");
        const i64 bufferSize = (i64)Buffer.Size();
        const i64 bufferOffset = filePosition - bufferSize;
        if (offset >= bufferOffset && offset + (i64)sizeof(TEntryHeader) <= filePosition) { // is header within the buffer?
            const i64 bufferPosition = offset - bufferOffset;
            char* p = Buffer.Data() + bufferPosition;
            TEntryHeader* header = reinterpret_cast<TEntryHeader*>(p);
            if (bufferPosition + (i64)header->SizeOf() <= bufferSize) // is record data within the buffer?
                return p;
        }
        return nullptr;
    }
public:
    TFileRecordIterator(size_t bufSize = 2000000)
        : Buffer((bufSize + FS_BLOCK_SIZE -1) & ~(FS_BLOCK_SIZE - 1))
        , CurPos(nullptr)
        , Eof(false)
    {
    }

    TFileRecordIterator(TBuffer& r) {
        Buffer.Swap(r);
        Buffer.Clear();
        CurPos = NULL;
    }

    ~TFileRecordIterator() {
        Close();
    }

    const TString& GetFileName() const {
        return File.GetName();
    }

    void Open(const char *name, EOpenMode mode = 0) {
        if (Buffer.Data() == nullptr)
            ythrow yexception() << "TFileRecordIterator: Memory not allocated";
        File = TFile(name, RdOnly | mode);
        Eof = false;
    }

    void Open(const TFile& other) {
        if (Buffer.Data() == NULL)
            ythrow yexception() << "TFileRecordIterator: Memory not allocated";
        File = other;
    }

    template<class TFileHeader>
    void Open(const char *name, TFileHeader& fh, EOpenMode mode = 0) {
        Open(name, mode);
        size_t rd = File.ReadOrFail(Buffer.Data(), Buffer.Avail());
        Eof = rd < Buffer.Avail();
        Buffer.Proceed(rd);
        TMemoryInput tempInput(Buffer.Data(), rd);
        fh.LoadHeader(tempInput);
        rd = tempInput.Buf() - Buffer.Data();
        CurPos = Buffer.Data() + rd;
    }

    void Close() {
        File.Close();
        Buffer.Clear();
        CurPos = nullptr;
    }

    //! @attention the beginning of a TEntryHeader must be located at the specified offset
    void Seek(i64 offset) {
        CurPos = CheckRecord(offset);
        if (CurPos)
            return; // no refill required
        File.Seek(offset, sSet);
        Eof = false; // it can seek back from eof (for example when reading unordered arc with -S)
        Buffer.Clear();
        RefillBuffer();
    }

    bool IsValidPtr(const TEntryHeader* e) const {
        const char* pos = (const char*)e;
        return pos >= Buffer.Begin() && pos + e->SizeOf() <= Buffer.End();
    }

    TEntryHeader* Next() {
        TEntryHeader* result = Current();
        if (!result)
            return nullptr;
        size_t len = result->SizeOf();
        if (CurPos + len > Buffer.End()) {
            return nullptr;
        }
        CurPos += len;
        Y_ASSERT(Buffer.End() >= CurPos);
        return result;
    }

    TEntryHeader* Current() {
        if (!CurPos)
            return nullptr;
        if (CurPos + sizeof(TEntryHeader) > Buffer.End())  // last document may have size=0 (IsFake)
            return nullptr;
        TEntryHeader* e = (TEntryHeader*)CurPos;
        return e;
    }

    bool RefillBuffer() {
        if (!File.IsOpen() || Eof)
            return false;
        ui32 bOff = 0;
        if (CurPos) {
            Y_ASSERT(Buffer.End() >= CurPos);
            size_t bufOff = Buffer.End() - CurPos;
            size_t off = (bufOff + FS_BLOCK_SIZE - 1) & ~(FS_BLOCK_SIZE - 1);
            if (bufOff)
                memmove(Buffer.Data() + (bOff = off - bufOff), CurPos, bufOff);
            Buffer.Proceed(off);
        } else {
            Y_ASSERT(Buffer.Size() == 0);
        }

        size_t toRead = Buffer.Avail();
        size_t read = File.ReadOrFail(Buffer.End(), toRead);
        if (read < toRead)
            Eof = true;
        if (!read) {
            if (Buffer.Size())
                ythrow yexception() << "Unrecognized data in file \"" <<  File.GetName().data() << "\"";
            CurPos = Buffer.Data() + bOff;
            return false;
        }
        Buffer.Proceed(Buffer.Size() + read);

        if (sizeof(TEntryHeader) > Buffer.Size())
            ythrow yexception() << "Unrecognized data in file \"" <<  File.GetName().data() << "\"";
        size_t curRecLen = ((TEntryHeader*)(Buffer.Data() + bOff))->SizeOf();
        if (curRecLen > Buffer.Size() - bOff) { // insufficient buffer size
            Buffer.Reserve((curRecLen + bOff + FS_BLOCK_SIZE - 1) & ~(FS_BLOCK_SIZE - 1));
            size_t read2 = File.ReadOrFail(Buffer.End(), Buffer.Avail());
            Buffer.Proceed(Buffer.Size() + read2);
            if (curRecLen > Buffer.Size()) {
                ythrow yexception() << "Unrecognized data in file \"" <<  File.GetName().data() << "\"";
            }
        }
        CurPos = Buffer.Data() + bOff;
        Y_ASSERT(Buffer.End() >= CurPos);
        return true;
    }
};
