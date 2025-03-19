#pragma once
#include <kernel/common_server/library/scheme/scheme.h>
#include <kernel/common_server/library/scheme/fields.h>
#include <kernel/common_server/library/scheme/operations.h>

namespace NFrontend {
    using TScheme = NCS::NScheme::TScheme;
    using IElement = NCS::NScheme::IElement;
    using ESchemeFormat = NCS::NScheme::ESchemeFormat;
    using EElementType = NCS::NScheme::EElementType;
    using TConstantsInfoReport = NCS::NScheme::TConstantsInfoReport;
}

using TFSArray = NCS::NScheme::TFSArray;
using TFSMap = NCS::NScheme::TFSMap;
using TFSString = NCS::NScheme::TFSString;
using TFSNumeric = NCS::NScheme::TFSNumeric;
using TFSDuration = NCS::NScheme::TFSDuration;
using TFSBoolean = NCS::NScheme::TFSBoolean;
using TFSIgnore = NCS::NScheme::TFSIgnore;
using TFSVariants = NCS::NScheme::TFSVariants;
using TFSWideVariants = NCS::NScheme::TFSWideVariants;
using TFSStructure = NCS::NScheme::TFSStructure;
