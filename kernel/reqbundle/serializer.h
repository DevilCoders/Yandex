#pragma once

#include "blocks_remapper.h"
#include "reqbundle.h"

#include <kernel/qtree/compressor/factory.h>
#include <kernel/lingboost/error_handler.h>

#include <util/string/builder.h>

#include <kernel/reqbundle/proto/reqbundle.pb.h>

namespace NReqBundle {
namespace NSer {
    using TRevFreqsArrayProto = ::google::protobuf::RepeatedPtrField<NReqBundleProtocol::TRevFreqValue>;

    class IBundleSerializer {
    public:
        virtual TString SerializeBundle(TConstReqBundleAcc bundle) const = 0;
        virtual ~IBundleSerializer() = default;
    };
    class IBundleDeserializer {
    public:
        virtual TReqBundlePtr DeserializeBundle(TStringBuf serialized) const = 0;
        virtual TReqBundlePtr DeserializeBundleWithNormedTexts(TStringBuf serialized) const {
            return DeserializeBundle(serialized);
        }
        virtual ~IBundleDeserializer() = default;
    };

    class TSerializer
        : public TNonCopyable
        , public IBundleSerializer
    {
    public:
        using EFormat = TCompressorFactory::EFormat;

        enum EBinarizationMode {
            PreferBinary = 0, // Use binary, when available
            PreferBlock = 1,  // Use (non-binary) block, when available
            EnforceBinary = 2 // Always binarize blocks using specified format
        };

        struct TOptions {
            EFormat Format = TCompressorFactory::LZ_LZ4;
            EFormat BlocksFormat = TCompressorFactory::LZ_LZ4;
            EBinarizationMode BinMode = PreferBinary;

            bool OrderBlocks = false;
            bool UseDeltaFormsEncoding = false;
        };

    public:
        static constexpr const size_t DefaultBufferSize = 256UL << 10;

    public:
        TSerializer() = default;

        explicit TSerializer(const TOptions& options)
            : Options(options)
        {}

        explicit TSerializer(EFormat format) {
            Options.Format = format;
            Options.BlocksFormat = format;
        }

        void SerializeToProto(TConstRequestAcc request, NReqBundleProtocol::TRequest& proto,
            NDetail::TBlocksRemapper* remap = nullptr) const;
        void SerializeToProto(TConstConstraintAcc constraint, NReqBundleProtocol::TConstraint& proto,
            NDetail::TBlocksRemapper* remap = nullptr) const;
        void SerializeToProto(TConstBlockAcc block, NReqBundleProtocol::TBlock& proto) const;
        void SerializeToProto(TConstBinaryBlockAcc binary, NReqBundleProtocol::TBlock& proto) const;
        void SerializeToProto(TConstSequenceAcc seq, NReqBundleProtocol::TReqBundle& proto,
            NDetail::TBlocksRemapper* remap = nullptr) const;
        void SerializeToProto(TConstReqBundleAcc bundle, NReqBundleProtocol::TReqBundle& proto) const;

        template <typename TProto>
        void SerializeProto(const TProto& proto, IOutputStream* output) const;

        TString SerializeBundle(TConstReqBundleAcc bundle) const override;
        void Serialize(TConstBlockAcc block, IOutputStream* output) const;
        void Serialize(TConstBinaryBlockAcc binary, IOutputStream* output) const;
        void Serialize(TConstSequenceAcc seq, IOutputStream* output) const;
        void Serialize(TConstRequestAcc request, IOutputStream* output) const;
        void Serialize(TConstReqBundleAcc bundle, IOutputStream* output) const;

        size_t GetBinaryHash(TConstRequestAcc request, bool ignoreFacets = true) const;

        void MakeBinary(TConstBlockAcc block, TBinaryBlockAcc binary) const;
        size_t GetBinaryHash(TConstBlockAcc block) const;

        template <typename TProto>
        TString SerializeProto(const TProto& proto) const;

        template <typename TObj>
        TString Serialize(const TObj& obj) const;

    private:
        void SerializeToProto(const NLingBoost::TRevFreq& revFreq, TRevFreqsArrayProto& proto) const;
        void SerializeToProto(const NDetail::TFormData& formData, size_t prefix, NReqBundleProtocol::TBlockFormInfo& proto) const;
        void SerializeToProto(const NDetail::TLemmaData& lemmaData, NReqBundleProtocol::TBlockLemmaInfo& proto) const;
        void SerializeToProto(const NDetail::TWordData& wordData, NReqBundleProtocol::TBlockWordInfo& proto) const;
        void SerializeToProto(const NDetail::TBlockData& blockData, NReqBundleProtocol::TBlock& proto) const;
        void SerializeToProto(const NDetail::TBinaryBlockData& binaryData, NReqBundleProtocol::TBlock& proto) const;
        void SerializeToProto(const NDetail::TSequenceData& seqData, NReqBundleProtocol::TReqBundle& proto,
            NDetail::TBlocksRemapper* remap = nullptr) const;
        void SerializeToProto(const NDetail::TMatchData& matchData, NReqBundleProtocol::TRequestMatch& proto,
            NDetail::TBlocksRemapper* remap = nullptr) const;
        void SerializeToProto(const NDetail::TConstraintData& constraintData, NReqBundleProtocol::TConstraint& proto,
            NDetail::TBlocksRemapper* remap = nullptr) const;
        void SerializeToProto(const NDetail::TFacetsData::TEntry& entryData, NReqBundleProtocol::TFacet& proto) const;
        void SerializeToProto(const NDetail::TRequestData& reqData, NReqBundleProtocol::TRequest& proto,
            NDetail::TBlocksRemapper* remap = nullptr) const;
        void SerializeToProto(const NDetail::TReqBundleData& bundleData, NReqBundleProtocol::TReqBundle& proto) const;
        void SerializeToProto(const TRequestTrCompatibilityInfo& info, NReqBundleProtocol::TRequestTrCompatibilityInfo& proto) const;

        void PrepareRawData(TConstBlockAcc block, TBuffer& buffer, size_t& hash) const;
        void PrepareRawData(TConstRequestAcc request, bool ignoreFacets, TBuffer& buffer, size_t& hash) const;

        template <typename TObj, typename TProto>
        void SerializeHelper(const TObj& obj, IOutputStream* output) const;

    private:
        TOptions Options;
        mutable TBuffer Buffer;
    };

    class TDeserializer
        : public TNonCopyable
        , public IBundleDeserializer
    {
        /*
         * Not thread-safe, contains mutable state
         */
    public:
        using EFailMode = NLingBoost::TErrorHandler::EFailMode;

        struct TOptions {
            // regardless of this setting,
            // protobuf parser will always throw
            EFailMode FailMode = EFailMode::SkipOnError;

            bool NeedAllData = false;

            TOptions() {}
        };

    public:
        explicit TDeserializer(const TOptions& options = {})
            : Options(options)
            , Handler(options.FailMode)
        {}

        bool IsInErrorState() const {
            return Handler.IsInErrorState();
        }
        void ClearErrorState() {
            Handler.ClearErrorState();
        }
        TString GetFullErrorMessage() const;

        void DeserializeProto(const NReqBundleProtocol::TBlock& proto, TBlockAcc block) const;
        void DeserializeProto(const NReqBundleProtocol::TBlock& proto, TBinaryBlockAcc binary) const;
        void DeserializeProto(const NReqBundleProtocol::TReqBundle& proto, TSequenceAcc seq) const;
        void DeserializeProto(const NReqBundleProtocol::TReqBundle& proto, TReqBundleAcc bundle) const;

        template <typename TProto>
        void DeserializeToProto(IInputStream* input, TProto& proto) const;

        TReqBundlePtr DeserializeBundle(TStringBuf serialized) const override;
        void Deserialize(IInputStream* input, TBlockAcc block) const;
        void Deserialize(IInputStream* input, TBinaryBlockAcc binary) const;
        void Deserialize(IInputStream* input, TSequenceAcc seq) const;
        void Deserialize(IInputStream* input, TReqBundleAcc bundle) const;

        void ParseBinary(TConstBinaryBlockAcc binary, TBlockAcc block) const;

        template <typename TProto>
        void DeserializeToProto(const TStringBuf& str, TProto& proto) const;

        template <typename TObj>
        void Deserialize(const TStringBuf& str, TObj& obj) const;

    private:
        void ValidateRevFreq(i64&) const;
        void ValidateAndDecodeLanguage(ui32, ELanguage&) const;
        void ValidateAndDecodeType(ui32, NDetail::EBlockType&) const;
        void ValidateAndDecodeCaseFlags(ui32, TCharCategory&) const;
        void ValidateAndDecodeNlpType(ui32, NLP_TYPE&) const;
        void ValidateAndDecodeConstraintType(ui32, EConstraintType&) const;
        void ValidateAndDecodeFormClass(ui32, EFormClass&) const;
        void ValidateMatchWeight(double&) const;
        void ValidateFacetValue(float&) const;
        void ValidateCohesion(float&) const;
        void ValidateRegionId(i64& regionId) const;

        void DeserializeProto(const TRevFreqsArrayProto& proto, NLingBoost::TRevFreq& revFreq) const;
        void DeserializeProto(const NReqBundleProtocol::TBlockFormInfo& proto, size_t prefixSize, const NDetail::TFormData& prevFormData, NDetail::TFormData& formData) const;
        void DeserializeProto(const NReqBundleProtocol::TBlockLemmaInfo& proto, NDetail::TLemmaData& lemmaData) const;
        void DeserializeProto(const NReqBundleProtocol::TBlockWordInfo& proto, NDetail::TWordData& wordData) const;
        void DeserializeProto(const NReqBundleProtocol::TBlock& proto, NDetail::TBlockData& blockData) const;
        void DeserializeProto(const NReqBundleProtocol::TBlock& proto, NDetail::TBinaryBlockData& binaryData) const;
        void DeserializeProto(const NReqBundleProtocol::TReqBundle& proto, NDetail::TSequenceData& seqData) const;
        void DeserializeProto(const NReqBundleProtocol::TRequestMatch& proto, NDetail::TMatchData& matchData) const;
        void DeserializeProto(const NReqBundleProtocol::TConstraint& proto, NDetail::TConstraintData& constraintData) const;
        void DeserializeProto(const NReqBundleProtocol::TFacet& proto, NDetail::TFacetsData::TEntry& entryData) const;
        void DeserializeProto(const NReqBundleProtocol::TRequest& proto, NDetail::TRequestData& reqData) const;
        void DeserializeProto(const NReqBundleProtocol::TReqBundle& proto, NDetail::TReqBundleData& bundleData) const;
        void DeserializeProto(const NReqBundleProtocol::TRequestTrCompatibilityInfo& proto, TRequestTrCompatibilityInfo& info) const;

        template <typename TObj, typename TProto>
        void DeserializeHelper(IInputStream* input, TObj& obj) const;

    private:
        TOptions Options;
        mutable NLingBoost::TErrorHandler Handler;
    };

    //
    //

    template <typename TProto>
    inline void TSerializer::SerializeProto(const TProto& proto, IOutputStream* output)  const
    {
        TCompressorFactory::Compress(output, proto.SerializeAsString(), Options.Format);
    }

    template <typename TObj, typename TProto>
    inline void TSerializer::SerializeHelper(const TObj& obj, IOutputStream* output)  const
    {
        TProto proto;
        SerializeToProto(obj, proto);
        SerializeProto(proto, output);
    }

    template <typename TProto>
    inline TString TSerializer::SerializeProto(const TProto& proto) const
    {
        TStringStream strOutput;
        SerializeProto(proto, &strOutput);
        return strOutput.Str();
    }

    template <typename TObj>
    inline TString TSerializer::Serialize(const TObj& obj) const
    {
        TStringStream strOutput;
        Serialize(obj, &strOutput);
        return strOutput.Str();
    }

    //
    //

    template <typename TProto>
    inline void TDeserializer::DeserializeToProto(IInputStream* input, TProto& proto) const
    {
        Y_ENSURE(proto.ParseFromString(TCompressorFactory::Decompress(input)),
            "proto deserialization failed");
    }

    template <typename TObj, typename TProto>
    inline void TDeserializer::DeserializeHelper(IInputStream* input, TObj& obj) const
    {
        TProto proto;
        DeserializeToProto(input, proto);
        DeserializeProto(proto, obj);
    }

    template <typename TProto>
    inline void TDeserializer::DeserializeToProto(const TStringBuf& str, TProto& proto) const
    {
        TMemoryInput memInput(str.data(), str.size());
        DeserializeToProto(&memInput, proto);
    }

    template <typename TObj>
    inline void TDeserializer::Deserialize(const TStringBuf& str, TObj& obj) const
    {
        Y_ENSURE(!str.empty());
        TMemoryInput memInput(str.data(), str.size());
        Deserialize(&memInput, obj);
    }
} // NSer
} // NReqBundle

