#pragma once

#include <nginx/modules/strm_packager/src/common/evp_cipher.h>

namespace NStrm::NPackager {
    // like TEvpCipher, but can return curent iv state
    //   and have AES-CTR with 8-byte IV (needed for CENC)
    class TCencCipher {
    public:
        virtual ~TCencCipher() = default;

        size_t KeyLength() const {
            return EvpCipher.KeyLength();
        }

        virtual size_t IVLength() const {
            return EvpCipher.IVLength();
        }

        size_t BlockSize() const {
            return EvpCipher.BlockSize();
        }

        virtual void EncryptInit(ui8 const* key, ui8 const* iv) = 0;

        virtual void GetIV(ui8* iv) const = 0;

        virtual void NextSample() = 0;

        // this always use full blocks
        //   i.e. in CBC mode inputSize must be multiple of 16 (= bock size)
        //   and  in CTR mode up to 15 (= block size - 1) bytes of last encrypted counter block are ignored
        virtual void Encrypt(ui8* output, ui8 const* input, size_t inputSize) = 0;

    protected:
        TCencCipher(EVP_CIPHER const* type);

    protected:
        TEvpCipher EvpCipher;
    };

    class TCencCipherAes128Ctr: public TCencCipher {
    public:
        TCencCipherAes128Ctr();
        void EncryptInit(ui8 const* key, ui8 const* iv) override;
        size_t IVLength() const override;
        void GetIV(ui8* iv) const override;
        void NextSample() override;
        void Encrypt(ui8* output, ui8 const* input, size_t inputSize) override;

    private:
        ui8 Key[16]; // need to store key for reinit EvpCipher
        ui64 IV;
        bool AtSampleStart;
    };

    class TCencCipherAes128Cbc: public TCencCipher {
    public:
        TCencCipherAes128Cbc();
        void EncryptInit(ui8 const* key, ui8 const* iv) override;
        void GetIV(ui8* iv) const override;
        void NextSample() override;
        void Encrypt(ui8* output, ui8 const* input, size_t inputSize) override;

    private:
        ui8 IV[16];
    };

}
