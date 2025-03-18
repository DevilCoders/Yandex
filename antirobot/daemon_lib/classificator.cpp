#include "classificator.h"
#include "matrixnet.h"
#include "catboost.h"

#include <util/stream/file.h>
#include <util/ysaveload.h>

namespace NAntiRobot {

namespace {

constexpr char MODEL_FILE_DESCRIPTOR_CHARS[4] = {'C', 'B', 'M', '1'};

ui32 GetModelFormatDescriptor() {
    ui32 result;
    memcpy(&result, MODEL_FILE_DESCRIPTOR_CHARS, sizeof(ui32));
    return result;
}

} // anonymous namespace

TClassificator<TProcessorLinearizedFactors>* CreateProcessorClassificator(const TString& formulaFilename) {
    bool isCatboost = false;

    try {
        ui32 header;
        TFileInput fin(formulaFilename);
        ::Load(&fin, header);
        isCatboost = (header == GetModelFormatDescriptor());
    } catch (yexception& e) {
        ythrow TClassificator<TProcessorLinearizedFactors>::TLoadError() << "Failed to load header from "
            << formulaFilename.Quote() << ":" << Endl << e.what();
    }

	if (isCatboost) {
		return new TCatboostClassificator<TProcessorLinearizedFactors>(formulaFilename);
	}
	return new TMatrixNetClassificator<TProcessorLinearizedFactors>(formulaFilename);
}

TClassificator<TCacherLinearizedFactors>* CreateCacherClassificator(const TString& formulaFilename) {
	return new TCatboostClassificator<TCacherLinearizedFactors>(formulaFilename);
}

template class TClassificator<TProcessorLinearizedFactors>;
template class TClassificator<TCacherLinearizedFactors>;

} // namespace NAntiRobot
