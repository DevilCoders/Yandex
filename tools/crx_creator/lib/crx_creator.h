#pragma once

#include <util/memory/blob.h>
#include <util/stream/input.h>

TBlob GetCrx3HeaderRSA(const TBlob& pem, IInputStream& archive);
