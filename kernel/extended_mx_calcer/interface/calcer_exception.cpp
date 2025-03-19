#include "calcer_exception.h"

const TString& TCalcerException::GetCalcerNotFoundMessage() {
    const static TString calcerNotFoundMessage = "Errored chain of subcalcers-descendants. The first one is missing, all others need it. ";
    return calcerNotFoundMessage;
}

TCalcerException::TCalcerException(const TString& error)
    :  ErrorMessage(error) {}

const char* TCalcerException::what() const noexcept {
    return ErrorMessage.c_str();
}

const TString& TCalcerException::GetCalcersCycleMessage() {
    const static TString calcersCycleMessage = "Errored chain of subcalcers-descendants. It contains cycle. ";
    return calcersCycleMessage;
}

TCalcerException& TCalcerException::AddCalcerToErroredChain(const TString& calcerName) {
    ErrorMessage.append(TStringBuilder() << calcerName << " <- ");
    return *this;
}
