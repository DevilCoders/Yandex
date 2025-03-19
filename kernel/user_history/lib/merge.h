#pragma once

#include <kernel/user_history/proto/user_history.pb.h>


namespace NPersonalization {

/*
  Patching user_history protobuf

  Behaviour:

  Fading embeddings
  + new will be added
  * matched will be replaced

  Records:
  All records in interval [-inf, max(timestamp of records)] will be replaced with records from patch

  Filtered records:
  * for matched descriptions: all records in interval [-inf, max(timestamp of records)] will be replaced with records from patch
  + new will be added
  timestamp extraction logic will respect SortOrderOnMirror field value
*/
[[nodiscard]]  NProto::TUserHistory Patch(const NPersonalization::NProto::TUserHistory& base, const NPersonalization::NProto::TUserHistory& patch, const NPersonalization::NProto::TUserHistoryPatch::EUserHistoryPatchLogic userDataPatchLogic=NPersonalization::NProto::TUserHistoryPatch::PreferDataFromBase);

[[nodiscard]]  NProto::TUserHistory Patch(const NPersonalization::NProto::TUserHistory& base, const NPersonalization::NProto::TUserHistoryPatch& patch);


[[nodiscard]] ::google::protobuf::RepeatedPtrField<::NPersonalization::NProto::TFadingEmbedding> Patch(
        const ::google::protobuf::RepeatedPtrField<::NPersonalization::NProto::TFadingEmbedding>& base,
        const ::google::protobuf::RepeatedPtrField<::NPersonalization::NProto::TFadingEmbedding>& patch,
        const NPersonalization::NProto::TUserHistoryPatch::EUserHistoryPatchLogic patchLogic);

}
