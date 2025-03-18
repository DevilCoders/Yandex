#include "coded_blob_utils.h"

#include <util/system/info.h>

#ifdef _unix_
#include <sys/mman.h>
#endif

namespace NCodedBlob {
    namespace NUtils {
        union TMagic {
            ui64 Value;
            char String[8];

            TMagic()
                : Value()
            {
            }
            TMagic(const char* v) {
                Set(v);
            }

            void Set(const char* v) {
                memcpy(String, v, CODED_BLOB_MAGIC_SIZE);
            }

            void Save(IOutputStream* out, ui64 version) const {
                ::Save(out, Value);
                ::Save(out, version);
            }

            ui64 CheckGetVersion(TBlob& b, ui64 maxversion) const {
                TMemoryInput min(b.AsCharPtr(), b.Size());
                TMagic somemagic;
                ::Load(&min, somemagic.Value);

                if (somemagic.Value != Value)
                    ythrow TCodedBlobException() << "bad magic: " << TStringBuf(somemagic.String, 8);

                ui64 version = -1;
                ::Load(&min, version);
                b = b.SubBlob(b.Size() - min.Avail(), b.Size());

                if (version > maxversion)
                    ythrow TCodedBlobException() << "bad version: " << version;

                return version;
            }
        };

        ui64 DoReadHeader(TBlob& b, const char* magic, ui64 maxversion) {
            return TMagic(magic).CheckGetVersion(b, maxversion);
        }

        TBlob DoSkipBody(TBlob& b) {
            TBlob body;
            ui64 footersz = 0;
            TMemoryInput min(b.End() - sizeof(ui64), sizeof(ui64));
            ::Load(&min, footersz);
            body = b.SubBlob(b.Size() - sizeof(ui64) - footersz);
            b = b.SubBlob(b.Size() - sizeof(ui64) - footersz, b.Size() - sizeof(ui64));
            return body;
        }

        void DoWriteHeader(IOutputStream* out, const char* magic, ui64 version) {
            NUtils::TMagic(magic).Save(out, version);
        }

        void DoWriteWithFooter(IOutputStream* out, const char* begin, ui64 size) {
            out->Write(begin, size);
            ::Save(out, (ui64)size);
        }

        void MoveToRam(TBlob& mmapped) {
            TBlob res = mmapped.DeepCopy();
#ifdef _unix_
            madvise(AlignUp((void*)mmapped.AsCharPtr(), NSystemInfo::GetPageSize()),
                    AlignDown(mmapped.Size(), NSystemInfo::GetPageSize()), MADV_DONTNEED);
#endif
            mmapped = res;
        }

        void SetRandomAccessed(TBlob mmapped) {
#ifdef _unix_
            // surprisingly, MADV_RANDOM does have its effect on the SSD mapping
            // according to iostat it significantly reduces rkB/s
            // local and remote load tests also consistenty demonstrate response times and rps improved by 0.1 and 500.
            madvise(AlignUp((void*)mmapped.AsCharPtr(), NSystemInfo::GetPageSize()),
                    AlignDown(mmapped.Size(), NSystemInfo::GetPageSize()), MADV_RANDOM);
#else
            Y_UNUSED(mmapped);
#endif
        }

    }
}
