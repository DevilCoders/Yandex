#pragma once

#include "abstract_progress_callback.h"

class IOutputStream;

namespace NDoom {

TAbstractProgressCallback* NewNullProgressCallback();
TAbstractProgressCallback* NewStreamProgressCallback(IOutputStream* stream);

} // namespace NDoom
