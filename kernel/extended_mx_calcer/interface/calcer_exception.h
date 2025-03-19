#pragma once
#include <util/generic/yexception.h>
#include <util/string/builder.h>

class TCalcerException : public yexception {
public:
    explicit TCalcerException(const TString& error);
    static const TString& GetCalcerNotFoundMessage();
    static const TString& GetCalcersCycleMessage();
    const char* what() const noexcept override;
    TCalcerException& AddCalcerToErroredChain(const TString& calcerName);

private:
    TString ErrorMessage;
};
