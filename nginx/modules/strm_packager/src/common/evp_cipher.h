#pragma once

extern "C" {
#include <ngx_event_openssl.h>
}

#include <util/generic/noncopyable.h>
#include <util/generic/yexception.h>
#include <util/system/types.h>

namespace NStrm::NPackager {
    class TEvpCipher: public TNonCopyable {
    public:
        // type like EVP_aes_128_cbc()
        TEvpCipher(EVP_CIPHER const* type);

        ~TEvpCipher();

        size_t KeyLength() const {
            return KeyLength_;
        }

        size_t IVLength() const {
            return IVLength_;
        }

        size_t BlockSize() const {
            return BlockSize_;
        }

        // init or reinit without cleaning
        void EncryptInit(ui8 const* key, ui8 const* iv, bool padding) {
            Init(EDirection::Encrypt, key, iv, padding);
        }
        void DecryptInit(ui8 const* key, ui8 const* iv, bool padding) {
            Init(EDirection::Decrypt, key, iv, padding);
        }

        // return numer of bytes written in output
        //  it can be from [0; inputSize + cipher_block_size]
        //  see EVP_EncryptUpdate / EVP_DecryptUpdate for details
        size_t EncryptUpdate(ui8* output, ui8 const* input, size_t inputSize) {
            return Update(EDirection::Encrypt, output, input, inputSize);
        }
        size_t DecryptUpdate(ui8* output, ui8 const* input, size_t inputSize) {
            return Update(EDirection::Decrypt, output, input, inputSize);
        }

        // return numer of bytes written in output
        //  it can be from [0; cipher_block_size]
        //  see EVP_EncryptFinal_ex for details
        size_t EncryptFinal(ui8* output) {
            return Final(EDirection::Encrypt, output);
        }
        size_t DecryptFinal(ui8* output) {
            return Final(EDirection::Decrypt, output);
        }

        size_t PredictEncryptedSize(const size_t inputSize) const {
            if (BlockSize_ == 1) {
                return inputSize;
            }

            if (Padding_) {
                return inputSize + BlockSize_ - (inputSize % BlockSize_);
            } else {
                Y_ENSURE(inputSize % BlockSize_ == 0);
                return inputSize;
            }
        }

        size_t PredictEncryptUpdate(const size_t inputSize) const {
            if (BlockSize_ == 1) {
                return inputSize;
            }

            size_t size = InputSizeCounter_ + inputSize;
            size -= size % BlockSize_;
            Y_ENSURE(size >= OutputSizeCounter_);
            return size - OutputSizeCounter_;
        }

        size_t PredictEncryptFinal() const {
            Y_ENSURE(!Finalized_);
            if (BlockSize_ == 1) {
                return 0;
            }

            const size_t size = PredictEncryptedSize(InputSizeCounter_);
            Y_ENSURE(size >= OutputSizeCounter_);
            return size - OutputSizeCounter_;
        }

    private:
        enum class EDirection {
            Unset,
            Encrypt,
            Decrypt,
        };

        // init or reinit without cleaning
        void Init(const EDirection dir, ui8 const* key, ui8 const* iv, bool padding);
        size_t Update(const EDirection dir, ui8* output, ui8 const* input, size_t inputSize);
        size_t Final(const EDirection dir, ui8* output);

        static inline size_t RoundUp(size_t x, size_t b) {
            return x + b - (x % b);
        }

        EVP_CIPHER const* const Type_;

        const size_t KeyLength_;
        const size_t IVLength_;
        const size_t BlockSize_;

        EVP_CIPHER_CTX* Context_;
        EDirection DirectionInit_;
        bool Padding_;
        bool Finalized_;
        size_t InputSizeCounter_;
        size_t OutputSizeCounter_;
    };

}
