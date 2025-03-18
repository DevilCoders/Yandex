#include "calc_module.h"
#include <util/string/split.h>

void MagicConnect(ICalcModule& module1, ICalcModule& module2, const TString& pointName) {
    ICalcModule::MagicConnect(module1, module2, pointName);
}
void MagicConnect(ICalcModule& module1, TCalcModuleHolder module2, const TString& pointName) {
    MagicConnect(module1, *module2, pointName);
}
void MagicConnect(TCalcModuleHolder module1, ICalcModule& module2, const TString& pointName) {
    MagicConnect(*module1, module2, pointName);
}
void MagicConnect(TCalcModuleHolder module1, TCalcModuleHolder module2, const TString& pointName) {
    MagicConnect(*module1, *module2, pointName);
}
void MagicConnect(ICalcModule& module1, const TString& pointName1, ICalcModule& module2, const TString& pointName2) {
    ICalcModule::MagicConnect(module1, pointName1, module2, pointName2);
}
void MagicConnect(ICalcModule& module1, const TString& pointName1, TCalcModuleHolder module2, const TString& pointName2) {
    MagicConnect(module1, pointName1, *module2, pointName2);
}
void MagicConnect(TCalcModuleHolder module1, const TString& pointName1, ICalcModule& module2, const TString& pointName2) {
    MagicConnect(*module1, pointName1, module2, pointName2);
}
void MagicConnect(TCalcModuleHolder module1, const TString& pointName1, TCalcModuleHolder module2, const TString& pointName2) {
    MagicConnect(*module1, pointName1, *module2, pointName2);
}
void MagicBatchConnect(ICalcModule& module1, ICalcModule& module2, TVector<TString> pointsNames) {
    for (const auto& pointsName : pointsNames)
        MagicConnect(module1, module2, pointsName);
}
void MagicBatchConnect(ICalcModule& module1, ICalcModule& module2, const TString& pointsNames) {
    TVector<TString> vec;
    StringSplitter(pointsNames.data()).Split(' ').SkipEmpty().Collect(&vec);
    MagicBatchConnect(module1, module2, vec);
}
void MagicBatchConnect(ICalcModule& module1, TCalcModuleHolder module2, const TString& pointsNames) {
    MagicBatchConnect(module1, *module2, pointsNames);
}
void MagicBatchConnect(TCalcModuleHolder module1, ICalcModule& module2, const TString& pointsNames) {
    MagicBatchConnect(*module1, module2, pointsNames);
}
void MagicBatchConnect(TCalcModuleHolder module1, TCalcModuleHolder module2, const TString& pointsNames) {
    MagicBatchConnect(*module1, *module2, pointsNames);
}
