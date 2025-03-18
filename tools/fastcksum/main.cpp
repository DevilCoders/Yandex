/*
 * Быстрый и менее точный вариант утилиты cksum.
 * Работает следующим образом:
 * считывает n байт из файла и считает crc64. скипает n байт и считывает n
 * байт, обновляя по ним crc. скипает 2*n байт и считывает снова n байт,
 * обновляя crc. И так далее до конца файла, каждый раз скипая в 2 раза
 * больше чем в предыдущий раз. Таким образом реально с диска считывается
 * очень немного данных, при этом обеспечивается неплохая точность. Нужно
 * это например для того чтобы быстро считать чексуммы от поисковой
 * базы(для определения человеческих ошибок - "что-то не туда скопировали",
 * ошибки диска так конечно не найти).
 */

#include <library/cpp/getopt/opt.h>

#include <library/cpp/deprecated/fgood/fgood.h>
#include <library/cpp/digest/old_crc/crc.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/generic/ylimits.h>
#include <util/stream/output.h>

#include <cstdlib>

// Assumed: BLOCKSIZE < M16
#define BLOCKSIZE 4096
#define M16     (16*1024*1024)

class TSum {
    public:
        typedef ui64 TResult;

        inline TSum() noexcept
            : Sum_(CRC64INIT)
        {
        }

        inline ~TSum() {
        }

        inline TString Result() noexcept {
            char tmp[30];
            snprintf(tmp,sizeof(tmp),"%020" PRIu64,Sum_);
            return TString (tmp);
        }

        inline void Update(const void* data, size_t len) noexcept {
            Sum_ = crc64(data, len, Sum_);
        }

    private:
        TResult Sum_;
};

static void Usage() {
    Cerr << "usage: fastcksum [-pqr] [-s string] [files ...]" << Endl;
    Cerr << "(BSD md5 compatible)" << Endl;
}


// Need to always sum the last block of file.
// Therefore it must be accessible after last seek.
template <class T>
class Seeker {
    typedef TVector<char> TBuf; // Need .swap()
    public:
        Seeker(T* stream, IOutputStream* ostream, bool stream_non_seekable)
            : Stream(stream)
            , OStream(ostream)
            , StreamNonSeekable(stream_non_seekable)
            , ReadLen(0)
            , BufferFileOffset(0)
            , FilePos(0)
            , BufReturned(true)
            , LastBlockReturned(false) {

            Buf.resize(M16);
        }

        void Proceed(ui64 off);
        char* Buffer() {
            return Buf.data();
        }
        size_t ReadLength() const {
            return ReadLen;
        }
        size_t ReadSpecial(size_t len) { // Always return last block in file, even if we've skipped a part of it.
            if (LastBlockReturned) { // For seekable stream
                ReadLen = 0;
                return 0;
            }
            TBuf buf1;
            if (len > Buf.size()) {
                Cerr << "Internal error: requested read length = " << len << " > maximum allowed " << Buf.size() << Endl;
                abort();
            }
            buf1.resize(Buf.size());
            ui64 newBufferFileOffset = FilePos;

            size_t readlen = Read(buf1.data(), len);

            if (OStream && readlen) {
                OStream->Write(buf1.data(), readlen);
            }

            if (readlen != len ) {
                if ((readlen > len) || (readlen == (size_t)-1)) { // Error
                    ReadLen = 0;
                    BufferFileOffset = newBufferFileOffset;
                } else { // readlen < len
                    if (BufReturned && StreamNonSeekable){
                        ReadLen = readlen;
                        BufferFileOffset = newBufferFileOffset;
                    } else if (!StreamNonSeekable) { // Seekable stream
                        SeekEndImpl(0); // Set proper FileOffset
                        i64 off = -(i64)len;
                        if (BufReturned && (BufferFileOffset + ReadLen >= FilePos)) { // Nothing to read;
                            off = 0;
                        } else if (BufReturned && (BufferFileOffset + ReadLen + len >= FilePos)) { // Can read less then wish
                            off = FilePos - (BufferFileOffset + ReadLen);
                            off = -off;
                        } else {
                        }
                        if (off < 0 ) {
                            SeekEnd(off);
                            readlen = Read(buf1.data(), len);
                            if (readlen <= len) {
                                ReadLen = readlen;
                            } else {
                                ReadLen = 0;
                            }
                        }
                        LastBlockReturned = true;
                    } else { // not BufReturned and non-seekable stream. So last data resides in current buffer.
                        size_t shift = len - readlen;
                        if (shift > ReadLen) {
                            shift = ReadLen;
                        }
                        memmove(buf1.data() + shift, buf1.data(), readlen);
                        memcpy(buf1.data(), Buffer() + ReadLen - shift, shift);
                        ReadLen = readlen + shift;
                        BufferFileOffset = newBufferFileOffset;
                    }
                }
            } else {
                ReadLen = readlen;
                BufferFileOffset = newBufferFileOffset;
            }
            Buf.swap(buf1);
            BufReturned = true;

            return ReadLen;
        }
    private:
        size_t Read(char * buf, size_t len); // Updates only FilePos.

        void SeekEndImpl(i64 off); // position offset (signed) bytes from SEEK_END

        void SeekEnd(i64 off) { // Only for seekable stream. Do not print data
            if (StreamNonSeekable) {
                ythrow yexception() << "SeekBack() called on non-seekable stream.";
            }
            SeekEndImpl(off);
        }

        // seek() stream by reading it and (optionally) copying to output stream
        void SeekByRead(ui64 off) {
            TBuf buf1;
            buf1.resize(Buf.size());

            size_t readlen = 1;
            while (off > 0 && readlen) {
                size_t to_read = off; // First, read smallest chunk, and then Buf.size() chunks.
                if (to_read > buf1.size()) {
                    if (to_read % buf1.size() != 0) {
                        to_read = to_read % buf1.size();
                    } else {
                        to_read = buf1.size();
                    }
                }
                readlen = Read(buf1.data(), to_read);
                off -= readlen;

                if (OStream && readlen) {
                    OStream->Write(buf1.data(), readlen);
                }
                size_t newReadLen;
                if (readlen < BLOCKSIZE) { // Leave at least BLOCKSIZE bytes in buffer
                    if ((!BufReturned) && (ReadLen > 0)) { // there are some usefull bytes in buffer
                        size_t shift = BLOCKSIZE - readlen;
                        if (shift > ReadLen) {
                            shift = ReadLen;
                        }
                        memmove(buf1.data() + shift, buf1.data(), readlen);
                        memcpy(buf1.data(), Buffer() + ReadLen - shift, shift);
                        newReadLen = readlen + shift;
                    } else {
                        newReadLen = readlen;
                    }
                } else {
                    newReadLen = readlen;
                }
                ReadLen = newReadLen ;
                BufferFileOffset = FilePos - ReadLen;
                Buf.swap(buf1);
                BufReturned = false;
            }
        }

        T* Stream;
        IOutputStream* OStream;
        bool StreamNonSeekable;
        TBuf Buf;
        size_t ReadLen;
        ui64 BufferFileOffset; // Offset start of buffer in current file
        ui64 FilePos; // Current file pointer
        bool BufReturned; // true, when current buffer has been returned to reader
        bool LastBlockReturned; // For seekable stream this flag means EOF;
};

template <>
void Seeker<IInputStream>::SeekEndImpl(i64) {
    ythrow yexception() << "SeekEndImpl(): IInputStream is not seekable.";
}

template <>
void Seeker<TStringInput>::SeekEndImpl(i64) {
    ythrow yexception() << "SeekEndImpl(): TStringInput is not seekable.";
}

template <>
void Seeker<FILE>::SeekEndImpl(i64 off) {
    if (StreamNonSeekable) {
        ythrow yexception() << "Seeker<FILE>::SeekEndImpl(): Stream is not seekable.";
    }
    if (off >= 0) {
        if (fseek(Stream,0, SEEK_END)) {
            ythrow yexception() << "can not seek to END in file(" << LastSystemErrorText() << ")";
        }
    } else {
        if (fseeko(Stream, off, SEEK_END)) {
            ythrow yexception() << "can not seek_END in file(" << LastSystemErrorText() << ")";
        }
    }
    FilePos = ftello(Stream);
}

template <>
void Seeker<IInputStream>::Proceed(ui64 off) {
    SeekByRead(off);
}

template <>
void Seeker<TStringInput>::Proceed(ui64 off) {
    SeekByRead(off);
}

template <>
void Seeker<FILE>::Proceed(ui64 off) {
    if ((OStream) || StreamNonSeekable) {
        SeekByRead(off);
    } else {
        if (fseeko(Stream, off, SEEK_CUR)) {
            ythrow yexception() << "can not seek in file(" << LastSystemErrorText() << ")";
        }
        FilePos = ftello(Stream);
    }
}

template <>
size_t Seeker<IInputStream>::Read(char *buf, size_t len) {
    size_t readlen = Stream->Load(buf, len);
    if (readlen <= len) {
        FilePos += readlen;
    }
    return readlen;
}

template <>
size_t Seeker<TStringInput>::Read(char *buf, size_t len) {
    size_t readlen = Stream->Load(buf, len);
    if (readlen <= len) {
        FilePos += readlen;
    }
    return readlen;
}

template <>
size_t Seeker<FILE>::Read(char *buf, size_t len) {
    size_t readlen = fread(buf, 1, len, Stream);
    if (readlen <= len) {
        FilePos += readlen;
    }
    return readlen;
}

// Process a stream
// (need stream_non_seekable because stdin is not seekable, though regular file is)
template <class F>
static inline void Process(F* file, TSum& sum, IOutputStream* o, const bool stream_non_seekable) {
    Seeker<F> seeker(file, o, stream_non_seekable);
    ui64 to_skip = BLOCKSIZE;
    size_t readed = 0;

    while ((readed = seeker.ReadSpecial(BLOCKSIZE)) != 0) {
        sum.Update(seeker.Buffer(), seeker.ReadLength());
        seeker.Proceed(to_skip); // seek()
        if (to_skip < M16) { // so that to_skip doesn't overflow INT_MAX
            to_skip *= 2;
        }
    }
}

static inline void ProcessOne(const char* path, const bool mode_quiet, const bool mode_reverse) {
    TFILEPtr file(fopen(path, "rb"));
    TSum sum;

    if (!file) {
        ythrow yexception() << "Unable to open input file (" << LastSystemErrorText() << ")";
    }

    Process((FILE*)file, sum, nullptr, false);

    if (mode_quiet) {
        Cout << sum.Result() << Endl;
    } else if (mode_reverse) {
        Cout << sum.Result() << " " << path << Endl;
    } else {
        Cout << "FASTCKSUM (" << path << ") = " << sum.Result() << Endl;
    }
}

static inline void ProcessString(const char* string, const bool mode_quiet, const bool mode_reverse) {
    const TString s(string);
    TStringInput strstream(s);
    TSum sum;

    Process(&strstream, sum, nullptr, true);

    if (mode_quiet) {
        Cout <<  sum.Result() << Endl;
    } else if (mode_reverse) {
        Cout << sum.Result() << " \"" << string << "\"" << Endl;
    } else {
        Cout << "FASTCKSUM (\"" << string << "\") = " << sum.Result() << Endl;
    }
}

static inline void ProcessStdin(const bool mode_p) {
    TSum sum;

    Process(stdin, sum, (mode_p ? &Cout : nullptr), true);

    Cout << sum.Result() << Endl;
}

int main(int argc, char** argv) {
    int ret = 0;

    // fastcksum fully complies to BSD md5(1)
    bool mode_q = false;
    bool mode_r = false;
    // -t && -x are not supported.

    // -------------------------------
    Opt opt(argc, argv, "s:pqrtx");

    int optlet;
    bool processed(false);

    while (EOF != (optlet = opt.Get())) {
        switch (optlet) {
            case 's':
                ProcessString(opt.GetArg(), mode_q, mode_r);
                processed = true;
                break;

            case 'p':
                ProcessStdin(true);
                processed = true;
                break;

            case 'q':
                mode_q = true;
                break;

            case 'r':
                mode_r = true;
                break;

            case 't':
                Cerr << "fastcksum: Built-in time-trial is not supported yet." << Endl;
                exit(1);

            case 'x':
                Cerr << "fastcksum: Self test script is not supported yet." << Endl;
                exit(1);

            default:
                //Cerr << "illegal option -- " << it->Key() << Endl;
                Usage();
                exit(1);
        }
    }

    for (int i = opt.Ind; i < argc; ++i) {
        try {
            ProcessOne(argv[i], mode_q, mode_r);
            processed = true;
        } catch (...) {
            Cerr << "fastcksum: " << argv[i] << ": " << CurrentExceptionMessage() << Endl;

            ++ret;
        }
    }

    // If nothing has been processed yet, default to stdin
    if (!processed) {
        ProcessStdin(false);
    }

    return ret;
}
