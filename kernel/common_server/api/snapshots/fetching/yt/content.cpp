#include "content.h"

namespace NCS {
    TYTSnapshotFetcher::TFactory::TRegistrator<TYTSnapshotFetcher> TYTSnapshotFetcher::Registrator(TYTSnapshotFetcher::GetTypeName());
}
