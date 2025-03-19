#include "encryptor.h"

#include "encryption_test.h"

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/keyring/endpoints_test.h>

#include <contrib/libs/openssl/include/openssl/err.h>
#include <contrib/libs/openssl/include/openssl/evp.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/guid.h>
#include <util/generic/scope.h>

namespace NCloud::NBlockStore {

namespace {

////////////////////////////////////////////////////////////////////////////////

const TString DefaultEncryptionKey
    = "0123456789012345678901234567890123456789012345678901234567890123";

////////////////////////////////////////////////////////////////////////////////

bool BlockFilledByZero(const TBlockDataRef& block)
{
    const char* ptr = block.Data();
    return (*ptr == 0) && memcmp(ptr, ptr + 1, block.Size() - 1) == 0;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TEncryptorTest)
{
    Y_UNIT_TEST(AesXtsEncryptorShouldEncryptDecryptBlock)
    {
        TEncryptionKeyFile keyFile(DefaultEncryptionKey);
        NProto::TKeyPath keyPath;
        keyPath.SetFilePath(keyFile.GetPath());

        ui64 blockIndex = 1234567;

        TString data = TString::TUninitialized(DefaultBlockSize);
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = static_cast<char>(i);
        }

        TString eData = TString::Uninitialized(DefaultBlockSize);
        TString dData = TString::Uninitialized(DefaultBlockSize);

        auto dataRef = TBlockDataRef{ data.data(), data.size() };
        auto eDataRef = TBlockDataRef{ eData.data(), eData.size() };
        auto dDataRef = TBlockDataRef{ dData.data(), dData.size() };

        auto encryptorOrError = CreateAesXtsEncryptor(keyPath);
        UNIT_ASSERT(!HasError(encryptorOrError));
        auto encryptor = encryptorOrError.ExtractResult();

        auto res1 = encryptor->Encrypt(dataRef, eDataRef, blockIndex);
        UNIT_ASSERT(res1 && dDataRef.Size() == eDataRef.Size() && eData != data);

        auto res2 = encryptor->Decrypt(eDataRef, dDataRef, blockIndex);
        UNIT_ASSERT(res2 && dData == data);
    }

    Y_UNIT_TEST(AesXtsEncryptorShouldDecryptZeroBlock)
    {
        TEncryptionKeyFile keyFile(DefaultEncryptionKey);
        NProto::TKeyPath keyPath;
        keyPath.SetFilePath(keyFile.GetPath());
        ui64 blockIndex = 1234567;

        auto eDataRef = TBlockDataRef::CreateZeroBlock(DefaultBlockSize);

        TString tmpData = TString::Uninitialized(DefaultBlockSize);
        auto dDataRef = TBlockDataRef{ tmpData.data(), tmpData.size() };

        auto encryptorOrError = CreateAesXtsEncryptor(keyPath);
        UNIT_ASSERT(!HasError(encryptorOrError));
        auto encryptor = encryptorOrError.ExtractResult();

        auto res = encryptor->Decrypt(eDataRef, dDataRef, blockIndex);
        UNIT_ASSERT(res && dDataRef.Size() == eDataRef.Size());
        UNIT_ASSERT(BlockFilledByZero(dDataRef));
    }

    Y_UNIT_TEST(AesXtsEncryptorShouldEncryptBlockUsingBlockIndex)
    {
        TEncryptionKeyFile keyFile(DefaultEncryptionKey);
        NProto::TKeyPath keyPath;
        keyPath.SetFilePath(keyFile.GetPath());

        ui64 blockIndex1 = 1234567;
        ui64 blockIndex2 = 7654321;

        TString data = TString::TUninitialized(DefaultBlockSize);
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = static_cast<char>(i);
        }

        TString eData1 = TString::Uninitialized(DefaultBlockSize);
        TString eData2 = TString::Uninitialized(DefaultBlockSize);

        auto dataRef = TBlockDataRef{ data.data(), data.size() };
        auto eDataRef1 = TBlockDataRef{ eData1.data(), eData1.size() };
        auto eDataRef2 = TBlockDataRef{ eData2.data(), eData2.size() };

        auto encryptorOrError = CreateAesXtsEncryptor(keyPath);
        UNIT_ASSERT(!HasError(encryptorOrError));
        auto encryptor = encryptorOrError.ExtractResult();

        {
            auto res1 = encryptor->Encrypt(dataRef, eDataRef1, blockIndex1);
            auto res2 = encryptor->Encrypt(dataRef, eDataRef2, blockIndex2);
            UNIT_ASSERT(res1 && res2 && eData1 != eData2);
        }

        TString dData1 = TString::Uninitialized(DefaultBlockSize);
        TString dData2 = TString::Uninitialized(DefaultBlockSize);

        auto dDataRef1 = TBlockDataRef{ dData1.data(), dData1.size() };
        auto dDataRef2 = TBlockDataRef{ dData2.data(), dData2.size() };

        auto res1 = encryptor->Decrypt(eDataRef1, dDataRef1, blockIndex1);
        UNIT_ASSERT(res1 && dData1 == data);

        auto res2 = encryptor->Decrypt(eDataRef2, dDataRef2, blockIndex2);
        UNIT_ASSERT(res2 && dData2 == data);
    }

    Y_UNIT_TEST(DefaultEncryptionKeyShouldProvideEmptyHash)
    {
        NProto::TEncryptionSpec spec;
        const auto& key = spec.GetKey();
        UNIT_ASSERT(key.GetMode() == NProto::NO_ENCRYPTION);

        auto hashOrError = ComputeEncryptionKeyHash(key);
        UNIT_ASSERT(!HasError(hashOrError) && hashOrError.GetResult().empty());
    }

    Y_UNIT_TEST(ShouldComputeEncryptionKeyHash)
    {
        NProto::TEncryptionKey spec;
        spec.SetMode(NProto::ENCRYPTION_AES_XTS);
        const TString encryptionKey
            = "0123456789012345678901234567890123456789012345678901234567890123";
        const TString expectedKeyHash
            = "46pv6YaiwYbFyt2l1RJvl/svleVOVYv5Jdp8WxZp/0hzMmV9oJ090nxkiMzXvr9g";
        const TString otherEncryptionKey
            = "1111111111111111111111111111111111111111111111111111111111111111";

        TEncryptionKeyFile keyFile1(encryptionKey, "test_key1");
        spec.MutableKeyPath()->SetFilePath(keyFile1.GetPath());
        auto hashOrError1 = ComputeEncryptionKeyHash(spec);
        UNIT_ASSERT_C(!HasError(hashOrError1), hashOrError1.GetError());
        UNIT_ASSERT_VALUES_EQUAL(
            expectedKeyHash,
            Base64Encode(hashOrError1.GetResult()));

        TEncryptionKeyFile keyFile2(otherEncryptionKey, "test_key2");
        spec.MutableKeyPath()->SetFilePath(keyFile2.GetPath());
        auto hashOrError2 = ComputeEncryptionKeyHash(spec);
        UNIT_ASSERT_C(!HasError(hashOrError2), hashOrError2.GetError());
        UNIT_ASSERT(hashOrError2.GetResult() != otherEncryptionKey);

        UNIT_ASSERT(hashOrError1.GetResult() != hashOrError2.GetResult());
    }

    Y_UNIT_TEST(ShouldFailEncryptorCreationIfKeyFileNotExist)
    {
        NProto::TKeyPath keyPath;
        keyPath.SetFilePath("nonexistent_file");

        {
            auto resultOrError = CreateAesXtsEncryptor(keyPath);
            UNIT_ASSERT(HasError(resultOrError));
        }

        {
            NProto::TEncryptionKey spec;
            spec.SetMode(NProto::ENCRYPTION_AES_XTS);
            spec.MutableKeyPath()->CopyFrom(keyPath);
            auto resultOrError = ComputeEncryptionKeyHash(spec);
            UNIT_ASSERT(HasError(resultOrError));
        }
    }

    Y_UNIT_TEST(ShouldFailEncryptorCreationIfFileKeyLengthIsInvalid)
    {
        TEncryptionKeyFile keyFile("key_with_invalid_length");

        NProto::TKeyPath keyPath;
        keyPath.SetFilePath(keyFile.GetPath());

        {
            auto resultOrError = CreateAesXtsEncryptor(keyPath);
            UNIT_ASSERT(HasError(resultOrError));
            UNIT_ASSERT_VALUES_EQUAL(
                E_ARGUMENT,
                resultOrError.GetError().GetCode());
        }

        {
            NProto::TEncryptionKey spec;
            spec.SetMode(NProto::ENCRYPTION_AES_XTS);
            spec.MutableKeyPath()->CopyFrom(keyPath);
            auto resultOrError = ComputeEncryptionKeyHash(spec);
            UNIT_ASSERT(HasError(resultOrError));
            UNIT_ASSERT_VALUES_EQUAL(
                E_ARGUMENT,
                resultOrError.GetError().GetCode());
        }
    }

    Y_UNIT_TEST(ShouldFailEncryptorCreationIfKeyRingNotExist)
    {
        NProto::TKeyPath keyPath;
        keyPath.SetKeyringId(-1);

        {
            auto resultOrError = CreateAesXtsEncryptor(keyPath);
            UNIT_ASSERT(HasError(resultOrError));
            UNIT_ASSERT_VALUES_EQUAL(
                E_ARGUMENT,
                resultOrError.GetError().GetCode());
        }

        {
            NProto::TEncryptionKey spec;
            spec.SetMode(NProto::ENCRYPTION_AES_XTS);
            spec.MutableKeyPath()->CopyFrom(keyPath);
            auto resultOrError = ComputeEncryptionKeyHash(spec);
            UNIT_ASSERT(HasError(resultOrError));
            UNIT_ASSERT_VALUES_EQUAL(
                E_ARGUMENT,
                resultOrError.GetError().GetCode());
        }
    }

    Y_UNIT_TEST(ShouldFailEncryptorCreationIfKeyPathIsEmpty)
    {
        NProto::TKeyPath emptyKeyPath;

        {
            auto resultOrError = CreateAesXtsEncryptor(emptyKeyPath);
            UNIT_ASSERT(HasError(resultOrError));
            UNIT_ASSERT_VALUES_EQUAL(
                E_ARGUMENT,
                resultOrError.GetError().GetCode());
        }

        {
            NProto::TEncryptionKey spec;
            spec.SetMode(NProto::ENCRYPTION_AES_XTS);
            spec.MutableKeyPath()->CopyFrom(emptyKeyPath);
            auto resultOrError = ComputeEncryptionKeyHash(spec);
            UNIT_ASSERT(HasError(resultOrError));
            UNIT_ASSERT_VALUES_EQUAL(
                E_ARGUMENT,
                resultOrError.GetError().GetCode());
        }
    }

    Y_UNIT_TEST(CaesarEncryptorShouldEncryptDecryptBlock)
    {
        TString data = TString::TUninitialized(DefaultBlockSize);
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = static_cast<char>(i);
        }

        TString eData = TString::Uninitialized(DefaultBlockSize);
        TString dData = TString::Uninitialized(DefaultBlockSize);

        auto dataRef = TBlockDataRef{ data.data(), data.size() };
        auto eDataRef = TBlockDataRef{ eData.data(), eData.size() };
        auto dDataRef = TBlockDataRef{ dData.data(), dData.size() };

        auto blockIndex = 13;
        auto encryptor = CreateTestCaesarEncryptor(42);

        auto res1 = encryptor->Encrypt(dataRef, eDataRef, blockIndex);
        UNIT_ASSERT(res1 && dDataRef.Size() == eDataRef.Size() && eData != data);

        auto res2 = encryptor->Decrypt(eDataRef, dDataRef, blockIndex);
        UNIT_ASSERT(res2 && dData == data);
    }

    Y_UNIT_TEST(ShouldGetTheSameEncryptionKeyHashesFromFileAndDEK)
    {
        TString fileKeyHash;
        {
            TEncryptionKeyFile keyFile(DefaultEncryptionKey);
            NProto::TEncryptionKey spec;
            spec.SetMode(NProto::ENCRYPTION_AES_XTS);
            spec.MutableKeyPath()->SetFilePath(keyFile.GetPath());

            auto hashOrError = ComputeEncryptionKeyHash(spec);
            UNIT_ASSERT_C(!HasError(hashOrError), hashOrError.GetError());
            fileKeyHash = hashOrError.GetResult();
            UNIT_ASSERT(!fileKeyHash.empty());
        }

        TString dekHash;
        {
            NProto::TEncryptionKey spec;
            spec.SetMode(NProto::ENCRYPTION_AES_XTS);
            spec.MutableKeyPath()->SetDEK(DefaultEncryptionKey);

            auto hashOrError = ComputeEncryptionKeyHash(spec);
            UNIT_ASSERT_C(!HasError(hashOrError), hashOrError.GetError());
            dekHash = hashOrError.GetResult();
            UNIT_ASSERT(!dekHash.empty());
        }

        UNIT_ASSERT_VALUES_EQUAL(fileKeyHash, dekHash);
    }
}

}   // namespace NCloud::NBlockStore
