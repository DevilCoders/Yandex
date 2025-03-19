#pragma once
// just to make precommit hook happy

////////////!!!WARNING!!!/////////////////////////////////////////////////
// Please, regenerate this file manually after changing matrixnet.fbs ////
////////////!!!WARNING!!!/////////////////////////////////////////////////


#ifndef FLATBUFFERS_GENERATED_MATRIXNET_NMATRIXNETIDL_H_
#define FLATBUFFERS_GENERATED_MATRIXNET_NMATRIXNETIDL_H_

#include "flatbuffers/flatbuffers.h"


namespace NMatrixnetIdl {

struct TFeature;
struct TKeyValue;
struct TMultiData;
struct TMultiClassificationData;
struct TMNSSEModel;

enum EFeatureDirection {
  EFeatureDirection_None = 0,
  EFeatureDirection_Left = 1,
  EFeatureDirection_Right = 2,
  EFeatureDirection_MIN = EFeatureDirection_None,
  EFeatureDirection_MAX = EFeatureDirection_Right
};

inline const char **EnumNamesEFeatureDirection() {
  static const char *names[] = { "None", "Left", "Right", nullptr };
  return names;
}

inline const char *EnumNameEFeatureDirection(EFeatureDirection e) { return EnumNamesEFeatureDirection()[static_cast<int>(e)]; }

MANUALLY_ALIGNED_STRUCT(4) TFeature FLATBUFFERS_FINAL_CLASS {
 private:
  uint32_t Index_;
  uint32_t Length_;

 public:
  TFeature(uint32_t _Index, uint32_t _Length)
    : Index_(flatbuffers::EndianScalar(_Index)), Length_(flatbuffers::EndianScalar(_Length)) { }

  uint32_t Index() const { return flatbuffers::EndianScalar(Index_); }
  uint32_t Length() const { return flatbuffers::EndianScalar(Length_); }
};
STRUCT_END(TFeature, 8);

struct TKeyValue FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_KEY = 4,
    VT_VALUE = 6
  };
  const flatbuffers::String *Key() const { return GetPointer<const flatbuffers::String *>(VT_KEY); }
  const flatbuffers::String *Value() const { return GetPointer<const flatbuffers::String *>(VT_VALUE); }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyFieldRequired<flatbuffers::uoffset_t>(verifier, VT_KEY) &&
           verifier.Verify(Key()) &&
           VerifyFieldRequired<flatbuffers::uoffset_t>(verifier, VT_VALUE) &&
           verifier.Verify(Value()) &&
           verifier.EndTable();
  }
};

struct TKeyValueBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_Key(flatbuffers::Offset<flatbuffers::String> Key) { fbb_.AddOffset(TKeyValue::VT_KEY, Key); }
  void add_Value(flatbuffers::Offset<flatbuffers::String> Value) { fbb_.AddOffset(TKeyValue::VT_VALUE, Value); }
  TKeyValueBuilder(flatbuffers::FlatBufferBuilder &_fbb) : fbb_(_fbb) { start_ = fbb_.StartTable(); }
  TKeyValueBuilder &operator=(const TKeyValueBuilder &);
  flatbuffers::Offset<TKeyValue> Finish() {
    auto o = flatbuffers::Offset<TKeyValue>(fbb_.EndTable(start_, 2));
    fbb_.Required(o, TKeyValue::VT_KEY);  // Key
    fbb_.Required(o, TKeyValue::VT_VALUE);  // Value
    return o;
  }
};

inline flatbuffers::Offset<TKeyValue> CreateTKeyValue(flatbuffers::FlatBufferBuilder &_fbb,
   flatbuffers::Offset<flatbuffers::String> Key = 0,
   flatbuffers::Offset<flatbuffers::String> Value = 0) {
  TKeyValueBuilder builder_(_fbb);
  builder_.add_Value(Value);
  builder_.add_Key(Key);
  return builder_.Finish();
}

struct TMultiData FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_DATABIAS = 4,
    VT_DATASCALE = 6,
    VT_DATA = 8
  };
  double DataBias() const { return GetField<double>(VT_DATABIAS, 0.0); }
  double DataScale() const { return GetField<double>(VT_DATASCALE, 1.0); }
  const flatbuffers::Vector<int32_t> *Data() const { return GetPointer<const flatbuffers::Vector<int32_t> *>(VT_DATA); }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<double>(verifier, VT_DATABIAS) &&
           VerifyField<double>(verifier, VT_DATASCALE) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_DATA) &&
           verifier.Verify(Data()) &&
           verifier.EndTable();
  }
};

struct TMultiDataBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_DataBias(double DataBias) { fbb_.AddElement<double>(TMultiData::VT_DATABIAS, DataBias, 0.0); }
  void add_DataScale(double DataScale) { fbb_.AddElement<double>(TMultiData::VT_DATASCALE, DataScale, 1.0); }
  void add_Data(flatbuffers::Offset<flatbuffers::Vector<int32_t>> Data) { fbb_.AddOffset(TMultiData::VT_DATA, Data); }
  TMultiDataBuilder(flatbuffers::FlatBufferBuilder &_fbb) : fbb_(_fbb) { start_ = fbb_.StartTable(); }
  TMultiDataBuilder &operator=(const TMultiDataBuilder &);
  flatbuffers::Offset<TMultiData> Finish() {
    auto o = flatbuffers::Offset<TMultiData>(fbb_.EndTable(start_, 3));
    return o;
  }
};

inline flatbuffers::Offset<TMultiData> CreateTMultiData(flatbuffers::FlatBufferBuilder &_fbb,
   double DataBias = 0.0,
   double DataScale = 1.0,
   flatbuffers::Offset<flatbuffers::Vector<int32_t>> Data = 0) {
  TMultiDataBuilder builder_(_fbb);
  builder_.add_DataScale(DataScale);
  builder_.add_DataBias(DataBias);
  builder_.add_Data(Data);
  return builder_.Finish();
}

struct TMultiClassificationData FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_CLASSVALUES = 4
  };
  const flatbuffers::Vector<double> *ClassValues() const { return GetPointer<const flatbuffers::Vector<double> *>(VT_CLASSVALUES); }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_CLASSVALUES) &&
           verifier.Verify(ClassValues()) &&
           verifier.EndTable();
  }
};

struct TMultiClassificationDataBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_ClassValues(flatbuffers::Offset<flatbuffers::Vector<double>> ClassValues) { fbb_.AddOffset(TMultiClassificationData::VT_CLASSVALUES, ClassValues); }
  TMultiClassificationDataBuilder(flatbuffers::FlatBufferBuilder &_fbb) : fbb_(_fbb) { start_ = fbb_.StartTable(); }
  TMultiClassificationDataBuilder &operator=(const TMultiClassificationDataBuilder &);
  flatbuffers::Offset<TMultiClassificationData> Finish() {
    auto o = flatbuffers::Offset<TMultiClassificationData>(fbb_.EndTable(start_, 1));
    return o;
  }
};

inline flatbuffers::Offset<TMultiClassificationData> CreateTMultiClassificationData(flatbuffers::FlatBufferBuilder &_fbb,
   flatbuffers::Offset<flatbuffers::Vector<double>> ClassValues = 0) {
  TMultiClassificationDataBuilder builder_(_fbb);
  builder_.add_ClassValues(ClassValues);
  return builder_.Finish();
}

struct TMNSSEModel FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_VALUES = 4,
    VT_FEATURES = 6,
    VT_DATAINDICES = 8,
    VT_SIZETOCOUNT = 10,
    VT_DATA = 12,
    VT_MISSEDVALUEDIRECTIONS = 14,
    VT_DATABIAS = 16,
    VT_DATASCALE = 18,
    VT_INFOMAP = 20,
    VT_MULTIDATA = 22
  };
  const flatbuffers::Vector<float> *Values() const { return GetPointer<const flatbuffers::Vector<float> *>(VT_VALUES); }
  const flatbuffers::Vector<const TFeature *> *Features() const { return GetPointer<const flatbuffers::Vector<const TFeature *> *>(VT_FEATURES); }
  const flatbuffers::Vector<uint32_t> *DataIndices() const { return GetPointer<const flatbuffers::Vector<uint32_t> *>(VT_DATAINDICES); }
  const flatbuffers::Vector<int32_t> *SizeToCount() const { return GetPointer<const flatbuffers::Vector<int32_t> *>(VT_SIZETOCOUNT); }
  const flatbuffers::Vector<int32_t> *Data() const { return GetPointer<const flatbuffers::Vector<int32_t> *>(VT_DATA); }
  const flatbuffers::Vector<int8_t> *MissedValueDirections() const { return GetPointer<const flatbuffers::Vector<int8_t> *>(VT_MISSEDVALUEDIRECTIONS); }
  double DataBias() const { return GetField<double>(VT_DATABIAS, 0.0); }
  double DataScale() const { return GetField<double>(VT_DATASCALE, 1.0); }
  const flatbuffers::Vector<flatbuffers::Offset<TKeyValue>> *InfoMap() const { return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<TKeyValue>> *>(VT_INFOMAP); }
  const flatbuffers::Vector<flatbuffers::Offset<TMultiData>> *MultiData() const { return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<TMultiData>> *>(VT_MULTIDATA); }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_VALUES) &&
           verifier.Verify(Values()) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_FEATURES) &&
           verifier.Verify(Features()) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_DATAINDICES) &&
           verifier.Verify(DataIndices()) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_SIZETOCOUNT) &&
           verifier.Verify(SizeToCount()) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_DATA) &&
           verifier.Verify(Data()) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_MISSEDVALUEDIRECTIONS) &&
           verifier.Verify(MissedValueDirections()) &&
           VerifyField<double>(verifier, VT_DATABIAS) &&
           VerifyField<double>(verifier, VT_DATASCALE) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_INFOMAP) &&
           verifier.Verify(InfoMap()) &&
           verifier.VerifyVectorOfTables(InfoMap()) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_MULTIDATA) &&
           verifier.Verify(MultiData()) &&
           verifier.VerifyVectorOfTables(MultiData()) &&
           verifier.EndTable();
  }
};

struct TMNSSEModelBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_Values(flatbuffers::Offset<flatbuffers::Vector<float>> Values) { fbb_.AddOffset(TMNSSEModel::VT_VALUES, Values); }
  void add_Features(flatbuffers::Offset<flatbuffers::Vector<const TFeature *>> Features) { fbb_.AddOffset(TMNSSEModel::VT_FEATURES, Features); }
  void add_DataIndices(flatbuffers::Offset<flatbuffers::Vector<uint32_t>> DataIndices) { fbb_.AddOffset(TMNSSEModel::VT_DATAINDICES, DataIndices); }
  void add_SizeToCount(flatbuffers::Offset<flatbuffers::Vector<int32_t>> SizeToCount) { fbb_.AddOffset(TMNSSEModel::VT_SIZETOCOUNT, SizeToCount); }
  void add_Data(flatbuffers::Offset<flatbuffers::Vector<int32_t>> Data) { fbb_.AddOffset(TMNSSEModel::VT_DATA, Data); }
  void add_MissedValueDirections(flatbuffers::Offset<flatbuffers::Vector<int8_t>> MissedValueDirections) { fbb_.AddOffset(TMNSSEModel::VT_MISSEDVALUEDIRECTIONS, MissedValueDirections); }
  void add_DataBias(double DataBias) { fbb_.AddElement<double>(TMNSSEModel::VT_DATABIAS, DataBias, 0.0); }
  void add_DataScale(double DataScale) { fbb_.AddElement<double>(TMNSSEModel::VT_DATASCALE, DataScale, 1.0); }
  void add_InfoMap(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<TKeyValue>>> InfoMap) { fbb_.AddOffset(TMNSSEModel::VT_INFOMAP, InfoMap); }
  void add_MultiData(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<TMultiData>>> MultiData) { fbb_.AddOffset(TMNSSEModel::VT_MULTIDATA, MultiData); }
  TMNSSEModelBuilder(flatbuffers::FlatBufferBuilder &_fbb) : fbb_(_fbb) { start_ = fbb_.StartTable(); }
  TMNSSEModelBuilder &operator=(const TMNSSEModelBuilder &);
  flatbuffers::Offset<TMNSSEModel> Finish() {
    auto o = flatbuffers::Offset<TMNSSEModel>(fbb_.EndTable(start_, 10));
    return o;
  }
};

inline flatbuffers::Offset<TMNSSEModel> CreateTMNSSEModel(flatbuffers::FlatBufferBuilder &_fbb,
   flatbuffers::Offset<flatbuffers::Vector<float>> Values = 0,
   flatbuffers::Offset<flatbuffers::Vector<const TFeature *>> Features = 0,
   flatbuffers::Offset<flatbuffers::Vector<uint32_t>> DataIndices = 0,
   flatbuffers::Offset<flatbuffers::Vector<int32_t>> SizeToCount = 0,
   flatbuffers::Offset<flatbuffers::Vector<int32_t>> Data = 0,
   flatbuffers::Offset<flatbuffers::Vector<int8_t>> MissedValueDirections = 0,
   double DataBias = 0.0,
   double DataScale = 1.0,
   flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<TKeyValue>>> InfoMap = 0,
   flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<TMultiData>>> MultiData = 0) {
  TMNSSEModelBuilder builder_(_fbb);
  builder_.add_DataScale(DataScale);
  builder_.add_DataBias(DataBias);
  builder_.add_MultiData(MultiData);
  builder_.add_InfoMap(InfoMap);
  builder_.add_MissedValueDirections(MissedValueDirections);
  builder_.add_Data(Data);
  builder_.add_SizeToCount(SizeToCount);
  builder_.add_DataIndices(DataIndices);
  builder_.add_Features(Features);
  builder_.add_Values(Values);
  return builder_.Finish();
}

inline const NMatrixnetIdl::TMNSSEModel *GetTMNSSEModel(const void *buf) { return flatbuffers::GetRoot<NMatrixnetIdl::TMNSSEModel>(buf); }

inline bool VerifyTMNSSEModelBuffer(flatbuffers::Verifier &verifier) { return verifier.VerifyBuffer<NMatrixnetIdl::TMNSSEModel>(); }

inline void FinishTMNSSEModelBuffer(flatbuffers::FlatBufferBuilder &fbb, flatbuffers::Offset<NMatrixnetIdl::TMNSSEModel> root) { fbb.Finish(root); }

}  // namespace NMatrixnetIdl

#endif  // FLATBUFFERS_GENERATED_MATRIXNET_NMATRIXNETIDL_H_
