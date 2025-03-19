#pragma once

namespace NMatrixnet {
    class TMnSseInfo;
}

class TConstFactorView;
class TFactorStorage;
class TFactorView;
class IFactorsInfo;

namespace NRelevFml {
    bool DeriveMatrixnet(const NMatrixnet::TMnSseInfo* matrixnet, const TFactorStorage& factorStorage, TFactorStorage& deriveWebPos, TFactorStorage& deriveWebNeg, const int stepCount = -1);
} // namespace NRelevFml
