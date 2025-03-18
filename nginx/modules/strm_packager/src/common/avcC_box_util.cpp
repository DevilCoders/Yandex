#include <nginx/modules/strm_packager/src/common/avcC_box_util.h>
#include <strm/media/transcoder/mp4muxer/io.h>

#include <util/generic/yexception.h>

namespace NStrm::NPackager::NAvcCBox {
    // avcC box description is in ISO 14496-15 (AVCDecoderConfigurationRecord)

    ui8 GetNaluSizeLength(const TBuffer& avcC) {
        Y_ENSURE(avcC.Size() >= 5);
        // get byte with lengthSizeMinusOne
        ui8 b = ((ui8 const*)avcC.Data())[4];

        Y_ENSURE((b & 0b11111100) == 0b11111100);

        return int(b & 0b11) + 1;
    }

    void SetNaluSizeLength(TBuffer& avcC, int newSizeLength) {
        Y_ENSURE(avcC.Size() >= 5);
        Y_ENSURE(newSizeLength >= 1 && newSizeLength <= 4);
        // set byte with lengthSizeMinusOne
        const ui8 b = 0b11111100 | ((newSizeLength - 1) & 0b11);
        ((ui8*)avcC.Data())[4] = b;
    }

    TSpsPpsRange GetSpsPps(const TBuffer& avcC) {
        TSpsPpsRange result;

        ui8 const* const data = (ui8 const*)avcC.Data();
        NStrm::NMP4Muxer::TBlobReader reader(data, avcC.Size());

        reader.SetPosition(5);
        const ui8 spsCountByte = ReadUI8(reader);
        Y_ENSURE((spsCountByte & 0b11100000) == 0b11100000);

        const int spsCount = spsCountByte & 0b00011111;
        Y_ENSURE(spsCount >= 1);

        for (int i = 0; i < spsCount; ++i) {
            const int spsLength = ReadUi16BigEndian(reader);
            result.Sps.push_back(TSimpleBlob((ui8*)data + reader.GetPosition(), spsLength));
            reader.SetPosition(reader.GetPosition() + spsLength);
        }

        const int ppsCount = ReadUI8(reader);
        Y_ENSURE(ppsCount >= 1);

        for (int i = 0; i < ppsCount; ++i) {
            const int ppsLength = ReadUi16BigEndian(reader);
            result.Pps.push_back(TSimpleBlob((ui8*)data + reader.GetPosition(), ppsLength));
            reader.SetPosition(reader.GetPosition() + ppsLength);
        }

        Y_ENSURE(result.Pps.back().end() - data <= (i64)avcC.Size());

        return result;
    }
}
