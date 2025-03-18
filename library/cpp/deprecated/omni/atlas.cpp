#include "atlas.h"

/* TFixedSizeContinuousAtlas relies on little endian byte order. This can be fixed. */
#ifndef _little_endian_
#error Expecting little endian byte order
#endif

namespace NOmni {
    //TODO implement optimal_map

    class TArrayKeys {
    public:
        static bool DoCheckKeys(TFilePointersIter& iter) {
            ui32 docId = 0;
            while (!iter.AtEnd()) {
                TFilePointer fptr = iter.GetAndAdvance();
                if (fptr.Key != docId)
                    return 0;
                ++docId;
            }
            return 1;
        }
    };

    class TMapKeys {
    public:
        static bool DoCheckKeys(TFilePointersIter& iter) {
            TFilePointer prev = iter.GetAndAdvance();
            while (!iter.AtEnd()) {
                TFilePointer cur = iter.GetAndAdvance();
                if (prev.Key >= cur.Key)
                    return 0;
                prev = cur;
            }
            return 1;
        }
    };

    class TNestedLayout {
    public:
        static bool DoCheckFileLayout(TFilePointersIter& iter) {
            Y_UNUSED(iter);
            return true;
        }
    };

    //No grandchildren allowed == no gaps allowed
    class TFlatOptimizedLayout {
    public:
        static bool DoCheckFileLayout(TFilePointersIter& iter) {
            TFilePointer prev = iter.GetAndAdvance();
            while (!iter.AtEnd()) {
                TFilePointer cur = iter.GetAndAdvance();
                if (prev.End() != cur.Offset)
                    return 0;
                prev = cur;
            }
            return 1;
        }
    };

    //similiar to TFlatOptimizedLayout but all objects are of the same size
    class TFixedSizeOptimizedLayout {
    public:
        static bool DoCheckFileLayout(TFilePointersIter& iter) {
            TFilePointer prev = iter.GetAndAdvance();
            size_t objectSize = prev.Length;
            while (!iter.AtEnd()) {
                TFilePointer cur = iter.GetAndAdvance();
                if (prev.End() != cur.Offset)
                    return 0;
                if (cur.Length != objectSize)
                    return 0;
                prev = cur;
            }
            return 1;
        }
    };

    template <typename TKeyControl, typename TLayoutControl>
    class IProperAtlas: public IAtlas {
    public:
        bool CheckKeys(TFilePointersIter& iter) const override {
            bool res = TKeyControl::DoCheckKeys(iter);
            iter.Restart();
            return res;
        }

        bool CheckFileLayout(TFilePointersIter& iter) const override {
            bool res = TLayoutControl::DoCheckFileLayout(iter);
            iter.Restart();
            return res;
        }
    };

    class TCheckedContinuousAtlas: public IProperAtlas<TArrayKeys, TNestedLayout> {
    public:
        TBlob DoArchive(TFilePointersIter& iter) const override {
            TStringStream dst;
            while (!iter.AtEnd()) {
                TFilePointer fptr = iter.GetAndAdvance();
                fptr.Save(&dst);
            }
            return TBlob::FromString(dst.Str()).DeepCopy();
        }
        TFilePointer DoGetKeyFilePtr(const ui8* data, ui64 dataLen, size_t key) const override {
            Y_ASSERT(dataLen % sizeof(TFilePointer) == 0);
            const ui8* ptr = data + key * sizeof(TFilePointer);
            TFilePointer filePtr = *reinterpret_cast<const TFilePointer*>(ptr);
            Y_ASSERT(filePtr.Key == key); //that's why it's called 'checked'
            return filePtr;
        }
        TFilePointer DoGetPosFilePtr(const ui8* data, ui64 dataLen, size_t pos) const override {
            return DoGetKeyFilePtr(data, dataLen, pos);
        }
        size_t DoGetSize(const ui8* /*data*/, ui64 dataLen) const override {
            Y_ASSERT(dataLen % sizeof(TFilePointer) == 0);
            return dataLen / sizeof(TFilePointer);
        }
    };

    class TSimpleContinuousAtlas: public IProperAtlas<TArrayKeys, TFlatOptimizedLayout> {
    public:
        TBlob DoArchive(TFilePointersIter& iter) const override {
            TStringStream dst;
            size_t end = -1;
            while (!iter.AtEnd()) {
                TFilePointer filePtr = iter.GetAndAdvance();
                Save(&dst, filePtr.Offset);
                end = filePtr.End();
            }
            Save(&dst, end);
            return TBlob::FromString(dst.Str()).DeepCopy();
        }
        TFilePointer DoGetKeyFilePtr(const ui8* data, ui64 dataLen, size_t key) const override {
            Y_ASSERT(dataLen % sizeof(ui64) == 0);
            Y_ASSERT(key + 1 < dataLen / sizeof(ui64));
            ui64 offset = GetOffset(data, key);
            ui64 next = GetOffset(data, key + 1);
            return TFilePointer(key, offset, next - offset);
        }
        ui64 GetOffset(const ui8* data, ui64 key) const {
            const ui8* ptr = data + key * sizeof(ui64);
            return *reinterpret_cast<const ui64*>(ptr);
        }
        TFilePointer DoGetPosFilePtr(const ui8* data, ui64 dataLen, size_t pos) const override {
            return DoGetKeyFilePtr(data, dataLen, pos);
        }
        size_t DoGetSize(const ui8* /*data*/, ui64 dataLen) const override {
            Y_ASSERT(dataLen % sizeof(ui64) == 0);
            return dataLen / sizeof(ui64) - 1; //-1 for end offset
        }
    };

    ui8 GetNumBitsToRepresent(ui64 num) {
        ui64 one = 1;
        if (num >= (one << 63))
            return 64;
        ui8 nbits = 0;
        while ((one << nbits) <= num)
            ++nbits;
        return nbits;
    }

    ui8 GetNumBytesToRepresent(ui64 num) {
        return (GetNumBitsToRepresent(num) + 7) / 8;
    }

    void BitSerializeValue(ui64 num, ui8 nbits, ui64& pos, ui8* buf) {
        assert(nbits);
        static_assert(sizeof(ui64) == 8, "");
        ui8 nskip = pos & 0x7;
        ui64 byteno = pos >> 3;
        ui8 saved = buf[byteno];
        assert(saved <= (1 << nskip));
        ui64 part1 = num << nskip;
        size_t nbytes = (nbits + nskip + 7) >> 3;
        memcpy(buf + byteno, &part1, Min(nbytes, sizeof(ui64)));
        if (nbits > 56) {
            ui8 part2 = reinterpret_cast<ui8*>(&num)[7];
            part2 >>= (8 - nskip);
            buf[byteno + 8] = part2;
        }
        buf[byteno] |= saved;
        pos += nbits;
    }

    ui64 BitDeserializeValue(const ui8* buf, ui64& pos, ui8 nbits, ui64 border) {
        static_assert(sizeof(ui64) == 8, "");
        ui8 nskip = pos & 0x7;
        ui64 byteno = pos >> 3;
        ui64 part1 = 0;
        if (byteno + sizeof(ui64) <= border) {
            part1 = *reinterpret_cast<const ui64*>(buf + byteno);
        } else {
            memcpy(&part1, buf + byteno, border - byteno);
        }
        part1 >>= nskip;
        ui8 part2 = 0;
        if (byteno + sizeof(ui64) < border)
            part2 = buf[byteno + sizeof(ui64)];
        part2 <<= (8 - nskip);
        ui64 res = part1;
        reinterpret_cast<ui8*>(&res)[7] |= part2;
        if (nbits < 64) {
            res &= ((1ull << static_cast<ui64>(nbits)) - 1);
        }
        pos += nbits;
        return res;
    }

    class TLenMapAtlas: public IProperAtlas<TMapKeys, TNestedLayout> {
    public:
        TBlob DoArchive(TFilePointersIter& iter) const override {
            ui64 maxKey = 0;
            ui64 maxOffset = 0;
            ui64 maxLen = 0;
            while (!iter.AtEnd()) {
                TFilePointer filePtr = iter.GetAndAdvance();
                maxKey = Max(maxKey, filePtr.Key);
                maxOffset = Max(maxOffset, filePtr.Offset);
                maxLen = Max(maxLen, filePtr.Length);
            }
            iter.Restart();

            ui8 keyBits = GetNumBitsToRepresent(maxKey);
            ui8 ofsBits = GetNumBitsToRepresent(maxOffset);
            ui8 lenBits = GetNumBitsToRepresent(maxLen);
            ui8 nbits = ofsBits + keyBits + lenBits;

            ui8 varintBuf[16];
            ui8 viBufLen = 0;
            NPrivate::EncodeVarint(iter.Size(), varintBuf, viBufLen);
            Y_VERIFY(viBufLen < 16);

            size_t nbytesNeeded = (nbits * iter.Size() + 7) / 8;
            nbytesNeeded += viBufLen;
            nbytesNeeded += 3; //for keyBits, ofsBits, lenBits
            TVector<ui8> buf(nbytesNeeded, 0);

            for (size_t i = 0; i < viBufLen; ++i) {
                buf[i] = varintBuf[i];
            }
            buf[viBufLen] = keyBits;
            buf[viBufLen + 1] = ofsBits;
            buf[viBufLen + 2] = lenBits;
            ui64 dstPos = (viBufLen + 3) * 8;

            while (!iter.AtEnd()) {
                TFilePointer filePtr = iter.GetAndAdvance();
                BitSerializeValue(filePtr.Key, keyBits, dstPos, buf.data());
                BitSerializeValue(filePtr.Offset, ofsBits, dstPos, buf.data());
                BitSerializeValue(filePtr.Length, lenBits, dstPos, buf.data());
            }
            Y_VERIFY(dstPos <= nbytesNeeded * 8);
            return TBlob::Copy(buf.data(), nbytesNeeded);
        }

        void DecodeHeader(const ui8* data, ui8* headerLen, ui64* nOffsets, ui8* keyBits, ui8* ofsBits, ui8* lenBits, ui8* nbits) const {
            ui8 viBufLen = 0;
            *nOffsets = NPrivate::DecodeVarint(data, viBufLen);
            *keyBits = data[viBufLen];
            *ofsBits = data[viBufLen + 1];
            *lenBits = data[viBufLen + 2];
            *nbits = *ofsBits + *keyBits + *lenBits;
            *headerLen = viBufLen + 3;
        }

        TFilePointer DoGetKeyFilePtr(const ui8* data, ui64 dataLen, size_t key) const override {
            ui64 nOffsets;
            ui8 keyBits, ofsBits, lenBits, nbits, headerLen;
            DecodeHeader(data, &headerLen, &nOffsets, &keyBits, &ofsBits, &lenBits, &nbits);
            ui64 beg = 0;
            ui64 end = nOffsets;
            while (1) {
                if (beg >= end)
                    return TFilePointer(-1, -1, -1);
                if (beg + 1 == end) {
                    ui64 bitPos = headerLen * 8 + beg * nbits;
                    ui64 foundKey = BitDeserializeValue(data, bitPos, keyBits, dataLen);
                    if (foundKey != key)
                        return TFilePointer(-1, -1, -1);
                    ui64 offset = BitDeserializeValue(data, bitPos, ofsBits, dataLen);
                    ui64 len = BitDeserializeValue(data, bitPos, lenBits, dataLen);
                    Y_ASSERT(((bitPos + 7) >> 3) <= dataLen);
                    return TFilePointer(key, offset, len);
                }
                ui64 m = (beg + end) / 2;
                ui64 bitPos = headerLen * 8 + m * nbits;
                ui64 foundKey = BitDeserializeValue(data, bitPos, keyBits, dataLen);
                if (key < foundKey) {
                    end = m;
                } else {
                    beg = m;
                }
            }
        }

        TFilePointer DoGetPosFilePtr(const ui8* data, ui64 dataLen, size_t pos) const override {
            ui64 nOffsets;
            ui8 keyBits, ofsBits, lenBits, nbits, headerLen;
            DecodeHeader(data, &headerLen, &nOffsets, &keyBits, &ofsBits, &lenBits, &nbits);
            ui64 srcPos = headerLen * 8 + nbits * pos;
            Y_ASSERT(pos < nOffsets);
            ui64 key = BitDeserializeValue(data, srcPos, keyBits, dataLen);
            ui64 offset = BitDeserializeValue(data, srcPos, ofsBits, dataLen);
            ui64 len = BitDeserializeValue(data, srcPos, lenBits, dataLen);
            Y_ASSERT(((srcPos + 7) >> 3) <= dataLen);
            return TFilePointer(key, offset, len);
        }

        size_t DoGetSize(const ui8* data, ui64 dataLen) const override {
            ui8 viBufLen = 0;
            size_t nOffsets = NPrivate::DecodeVarint(data, viBufLen);
            Y_ASSERT(viBufLen < dataLen);
            return nOffsets;
        }
    };

    class TOptimalContinuousAtlas: public IProperAtlas<TArrayKeys, TFlatOptimizedLayout> {
    public:
        //good for small variable-sized objects
        TBlob DoArchive(TFilePointersIter& iter) const override {
            ui64 endOffset = iter.GetEndOffset();
            ui8 nbits = GetNumBitsToRepresent(endOffset);
            ui8 varintBuf[16];
            ui8 viBufLen = 0;
            NPrivate::EncodeVarint(iter.Size(), varintBuf, viBufLen);
            Y_VERIFY(viBufLen < 16);

            size_t nbytesNeeded = (nbits * (iter.Size() + 1) + 7) / 8; //+1 for endOffset
            nbytesNeeded += viBufLen;
            nbytesNeeded += 1; //for nbits
            TVector<ui8> buf(nbytesNeeded, 0);

            for (size_t i = 0; i < viBufLen; ++i) {
                buf[i] = varintBuf[i];
            }

            buf[viBufLen] = nbits;
            ui64 dstPos = (viBufLen + 1) * 8;
            while (!iter.AtEnd()) {
                TFilePointer filePtr = iter.GetAndAdvance();
                BitSerializeValue(filePtr.Offset, nbits, dstPos, buf.data());
            }
            BitSerializeValue(endOffset, nbits, dstPos, buf.data());
            Y_VERIFY(dstPos <= nbytesNeeded * 8);
            return TBlob::Copy(buf.data(), nbytesNeeded);
        }

        TFilePointer DoGetKeyFilePtr(const ui8* data, ui64 dataLen, size_t key) const override {
            ui8 viBufLen = 0;
            size_t nOffsets = NPrivate::DecodeVarint(data, viBufLen);
            Y_ASSERT(key < nOffsets);
            ui8 nbits = data[viBufLen];
            ui64 srcPos = (viBufLen + 1) * 8 + nbits * key;
            ui64 offset = BitDeserializeValue(data, srcPos, nbits, dataLen);
            ui64 next = BitDeserializeValue(data, srcPos, nbits, dataLen);
            Y_ASSERT(((srcPos + 7) >> 3) <= dataLen);
            return TFilePointer(key, offset, next - offset);
        }

        TFilePointer DoGetPosFilePtr(const ui8* data, ui64 dataLen, size_t pos) const override {
            return DoGetKeyFilePtr(data, dataLen, pos);
        }

        size_t DoGetSize(const ui8* data, ui64 dataLen) const override {
            ui8 viBufLen = 0;
            size_t nOffsets = NPrivate::DecodeVarint(data, viBufLen);
            Y_ASSERT(viBufLen < dataLen);
            return nOffsets;
        }
    };

    class TLenContinuousAtlas: public IProperAtlas<TArrayKeys, TNestedLayout> {
    public:
        TBlob DoArchive(TFilePointersIter& iter) const override {
            ui64 endOffset = iter.GetEndOffset();
            ui8 ofsBits = GetNumBitsToRepresent(endOffset);
            ui64 maxLen = 0;
            while (!iter.AtEnd()) {
                TFilePointer filePtr = iter.GetAndAdvance();
                maxLen = Max(maxLen, filePtr.Length);
            }
            iter.Restart();
            ui8 lenBits = GetNumBitsToRepresent(maxLen);
            ui8 varintBuf[16];
            ui8 viBufLen = 0;
            NPrivate::EncodeVarint(iter.Size(), varintBuf, viBufLen);
            Y_VERIFY(viBufLen < 16);
            ui8 nbits = ofsBits + lenBits;

            size_t nbytesNeeded = (nbits * iter.Size() + 7) / 8;
            nbytesNeeded += viBufLen;
            nbytesNeeded += 2; //for ofsBits and lenBits
            TVector<ui8> buf(nbytesNeeded, 0);

            for (size_t i = 0; i < viBufLen; ++i) {
                buf[i] = varintBuf[i];
            }

            buf[viBufLen] = ofsBits;
            buf[viBufLen + 1] = lenBits;
            ui64 dstPos = (viBufLen + 2) * 8;
            while (!iter.AtEnd()) {
                TFilePointer filePtr = iter.GetAndAdvance();
                BitSerializeValue(filePtr.Offset, ofsBits, dstPos, buf.data());
                BitSerializeValue(filePtr.Length, lenBits, dstPos, buf.data());
            }
            Y_VERIFY(dstPos <= nbytesNeeded * 8);
            return TBlob::Copy(buf.data(), nbytesNeeded);
        }

        TFilePointer DoGetKeyFilePtr(const ui8* data, ui64 dataLen, size_t key) const override {
            ui8 viBufLen = 0;
            size_t nOffsets = NPrivate::DecodeVarint(data, viBufLen);
            Y_ASSERT(key < nOffsets);
            ui8 ofsBits = data[viBufLen];
            ui8 lenBits = data[viBufLen + 1];
            ui8 nbits = ofsBits + lenBits;
            ui64 srcPos = (viBufLen + 2) * 8 + nbits * key;
            ui64 offset = BitDeserializeValue(data, srcPos, ofsBits, dataLen);
            ui64 len = BitDeserializeValue(data, srcPos, lenBits, dataLen);
            Y_ASSERT(((srcPos + 7) >> 3) <= dataLen);
            return TFilePointer(key, offset, len);
        }
        TFilePointer DoGetPosFilePtr(const ui8* data, ui64 dataLen, size_t pos) const override {
            return DoGetKeyFilePtr(data, dataLen, pos);
        }
        size_t DoGetSize(const ui8* data, ui64 dataLen) const override {
            ui8 viBufLen = 0;
            size_t nOffsets = NPrivate::DecodeVarint(data, viBufLen);
            Y_ASSERT(viBufLen < dataLen);
            return nOffsets;
        }
    };

    class TFixedSizeContinuousAtlas: public IProperAtlas<TArrayKeys, TFixedSizeOptimizedLayout> {
    public:
        //optimal for small fixed size objects like integers, or integer tuples
        TBlob DoArchive(TFilePointersIter& iter) const override {
            Y_VERIFY(!iter.AtEnd());
            TFilePointer filePtr = iter.GetAndAdvance();
            ui64 nobjects = iter.Size();
            ui32 objectSize = filePtr.Length;
            ui64 baseOffset = filePtr.Offset;
            ui8 signatureMask = 0; // 2 3 3
            ui8 szBytes = GetNumBytesToRepresent(objectSize);
            ui8 nobjBytes = GetNumBytesToRepresent(nobjects);
            ui8 baseOffsBytes = GetNumBytesToRepresent(baseOffset);
            Y_VERIFY(szBytes <= 4 && nobjBytes <= 8 && baseOffsBytes <= 8);
            signatureMask |= ((szBytes - 1) << 0);
            signatureMask |= ((nobjBytes - 1) << 2);
            signatureMask |= ((baseOffsBytes - 1) << 5);
            char buf[21]; //const size space needed!
            size_t bufLen = 1 + szBytes + nobjBytes + baseOffsBytes;
            Y_VERIFY(bufLen <= Y_ARRAY_SIZE(buf));
            buf[0] = signatureMask;
            memcpy(buf + 1, &objectSize, szBytes); //little endian!
            memcpy(buf + 1 + szBytes, &nobjects, nobjBytes);
            memcpy(buf + 1 + szBytes + nobjBytes, &baseOffset, baseOffsBytes);
            return TBlob::Copy(buf, bufLen);
        }
        TFilePointer DoGetKeyFilePtr(const ui8* data, ui64 dataLen, size_t key) const override {
            Y_ASSERT(dataLen <= 21);
            ui32 objectSize;
            ui64 nobjects, baseOffset;
            ReadHeader(data, objectSize, nobjects, baseOffset);
            Y_ASSERT(key < nobjects);
            ui64 offset = baseOffset + key * objectSize;
            return TFilePointer(key, offset, objectSize);
        }
        TFilePointer DoGetPosFilePtr(const ui8* data, ui64 dataLen, size_t pos) const override {
            return DoGetKeyFilePtr(data, dataLen, pos);
        }
        size_t DoGetSize(const ui8* data, ui64 dataLen) const override {
            Y_ASSERT(dataLen <= 21);
            ui32 objectSize;
            ui64 nobjects, baseOffset;
            ReadHeader(data, objectSize, nobjects, baseOffset);
            return nobjects;
        }

    private:
        void ReadHeader(const ui8* data, ui32& objectSize, ui64& nobjects, ui64& baseOffset) const {
            ui8 signatureMask = data[0];
            ui8 szBytes = (signatureMask & 0x03) + 1;
            ui8 nobjBytes = ((signatureMask & 0x1C) >> 2) + 1;
            ui8 baseOffsBytes = ((signatureMask & (ui8)0xE0) >> 5) + 1;

            objectSize = *reinterpret_cast<const ui32*>(data + 1);
            objectSize &= (((ui32)1 << (szBytes * 8)) - 1);

            nobjects = *reinterpret_cast<const ui64*>(data + 1 + szBytes);
            nobjects &= (((ui64)1 << (nobjBytes * 8)) - 1);

            baseOffset = *reinterpret_cast<const ui64*>(data + 1 + szBytes + nobjBytes);
            baseOffset &= (((ui64)1 << (baseOffsBytes * 8)) - 1);
        }
    };

    IAtlas* NewAtlas(EAtlasType atlasType) {
        switch (atlasType) {
            case AT_SIMPLE:
                return new TSimpleContinuousAtlas();
            case AT_CHECKED:
                return new TCheckedContinuousAtlas();
            case AT_ARR_FLAT:
                return new TOptimalContinuousAtlas();
            case AT_ARR_NESTED:
                return new TLenContinuousAtlas();
            case AT_MAP_NESTED:
                return new TLenMapAtlas();
            case AT_ARR_FIXED_SIZE:
                return new TFixedSizeContinuousAtlas();
            default:
                return nullptr; //for data nodes
        }
    }

}
