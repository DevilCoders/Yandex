#pragma once

#include <robot/jupiter/protos/extarc/extarc_keys.pb.h>

#include <util/generic/vector.h>
#include <util/memory/blob.h>

TBlob DecompressExtArc(TBlob blob, TConstArrayRef<TString> keys);

TBlob CompressExtArc(TBlob blob, TVector<NJupiter::TExtArcKey> keys);

TVector<TString> ExtractExtArcKeys(TBlob blob);
