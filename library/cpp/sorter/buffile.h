#pragma once

#include <sys/types.h>

#include <util/system/filemap.h>
#include <util/system/defaults.h>
#include <util/stream/input.h>

#include <errno.h>

template <class C, int size = 0x800000>
class CBufferedReadFile {
public:
    TMappedAllocation* m_Map;
    ui32 m_bufSize;
    ui32 m_current;
    TFile m_file;

public:
    CBufferedReadFile()
        : m_Map(nullptr)
        , m_bufSize(0)
        , m_current(0)
    {
    }
    ~CBufferedReadFile() {
        delete m_Map;
    }
    bool Next(C* elem) {
        if (m_current >= m_bufSize) {
            ui32 sz = size / sizeof(C);
            if (m_Map == nullptr)
                m_Map = new TMappedAllocation(sz * sizeof(C));
            if (m_Map == nullptr || m_Map->Ptr() == nullptr) // out of memory
                return false;
            m_current = 0;
            try {
                size_t r = m_file.Read(m_Map->Ptr(), sz * sizeof(C));
                if (r == 0)
                    return false;
                m_bufSize = r / sizeof(C);
            } catch (...) {
                fprintf(stderr, "Error in read, errno: %i\n", errno);
                return false;
            }
        }
        C* buf = (C*)m_Map->Ptr();
        *elem = buf[m_current++];
        return true;
    }
    void Finish() {
        if (m_file.IsOpen()) {
            TFileHandle temp(m_file.GetHandle());
            temp.Resize(0);
            temp.Release();
            m_file.Close();
        }
    }
    void SeekToBegin() {
        m_file.Seek(0, sSet);
        m_current = 0;
        m_bufSize = 0;
    }
    bool SetFile(const char* name) {
        try {
            m_file = TFile(name, RdOnly);
        } catch (const yexception& e) {
            fprintf(stderr, "Can't open file %s: %s\n", name, e.what());
            return false;
        }
        return true;
    }
};

template <class C, int size = 0x100000>
class CBufferedDirectReadFile: public CBufferedReadFile<C, size> {
public:
    ui32 m_BlockSize;
    bool m_eof;

public:
    using CBufferedReadFile<C, size>::m_Map;
    using CBufferedReadFile<C, size>::m_current;
    using CBufferedReadFile<C, size>::m_file;
    using CBufferedReadFile<C, size>::m_bufSize;

public:
    CBufferedDirectReadFile()
        : m_eof(false)
    {
        FindBlockSize();
    }

    void FindBlockSize() {
        m_BlockSize = sizeof(C);
        int sh = 0;
        while ((m_BlockSize & 1) == 0) {
            m_BlockSize >>= 1;
            sh++;
        }
        m_BlockSize <<= ::Max(9, sh);
    }

    void SeekToBegin() {
        CBufferedReadFile<C, size>::SeekToBegin();
        m_file.SetDirect();
        m_eof = false;
    }

    bool RefillBuffer() {
        if (m_eof)
            return false;
        ui32 sz = (size - (size % m_BlockSize)) / sizeof(C);
        if (m_Map == nullptr)
            m_Map = new TMappedAllocation(sz * sizeof(C));
        if (m_Map == nullptr || m_Map->Ptr() == nullptr) // out of memory
            return false;
        m_current = 0;
        try {
            size_t r = m_file.ReadOrFail(m_Map->Ptr(), sz * sizeof(C));
            if (r < sz * sizeof(C))
                m_eof = true;
            if (r == 0)
                return false;
            m_bufSize = r / sizeof(C);
            return true;
        } catch (...) {
            fprintf(stderr, "Error in read, errno: %i\n", errno);
            return false;
        }
    }

    bool Next(C* elem) {
        if (m_current >= m_bufSize) {
            if (!RefillBuffer())
                return false;
        }
        C* buf = (C*)m_Map->Ptr();
        *elem = buf[m_current++];
        return true;
    }

    ui32 Next(C* elem, ui32 count) {
        ui32 scount = count;
        while (count) {
            if (m_current >= m_bufSize) {
                if (!RefillBuffer())
                    return scount - count;
            }
            ui32 avail = m_bufSize - m_current;
            ui32 portion = ::Min<ui32>(avail, count);
            C* buf = (C*)m_Map->Ptr();
            memcpy(elem, buf + m_current, portion * sizeof(C));
            m_current += portion;
            elem += portion;
            count -= portion;
        }
        return scount - count;
    }
};

template <int size>
class TBufferedDirectInput: public IInputStream {
public:
    CBufferedDirectReadFile<char, size> File;

public:
    TBufferedDirectInput(const char* name) {
        File.SetFile(name);
        File.SeekToBegin();
    }

    virtual size_t DoRead(void* ptr, size_t len) {
        return File.Next((char*)ptr, len);
    }

    virtual size_t DoSkip(size_t len) {
        fprintf(stderr, "DoSkip(%lu) called\n", len);
        //        return File.Seek(len, SEEK_CUR);
        return 0;
    }
};
