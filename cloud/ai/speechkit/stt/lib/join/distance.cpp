//
// Created by Mikhail Yutman on 29.04.2020.
//

#include "distance.h"

#include <math.h>

TVector<TVector<ui32>> TLevenshteinDistance::CalculateAllDistancesBackward(const TText& ref1_, const TText& ref2_) const {
    TString ref1 = ref1_.JoinWithWhitespaces();
    TString ref2 = ref2_.JoinWithWhitespaces();
    TVector<TVector<ui32>> dp(ref1.length() + 1, TVector<ui32>(ref2.length() + 1));
    for (int i = ref2.length(); i >= 0; i--) {
        dp[ref1.length()][i] = ref2.length() - i;
    }
    for (int i = (int)ref1.length() - 1; i >= 0; i--) {
        dp[i][ref2.length()] = ref1.length() - i;
        for (int j = (int)ref2.length() - 1; j >= 0; j--) {
            dp[i][j] = std::min(dp[i + 1][j], dp[i][j + 1]) + 1;
            dp[i][j] = std::min(dp[i][j], dp[i + 1][j + 1] + (ref1[i] != ref2[j]));
        }
    }

    TVector<TVector<ui32>> ans(ref1_.Size() + 1, TVector<ui32>(ref2_.Size() + 1));
    ui32 ref1CharIndex = ref1.length();
    for (int i = ref1_.Size(); i >= 0; i--) {
        ui32 ref2CharIndex = ref2.length();
        for (int j = ref2_.Size(); j >= 0; j--) {
            ans[i][j] = dp[ref1CharIndex][ref2CharIndex];
            if (j > 0) {
                ref2CharIndex -= ref2_.Words[j - 1].length() + (ref2CharIndex != (ui32)ref2.length());
            }
        }
        if (i > 0) {
            ref1CharIndex -= ref1_.Words[i - 1].length() + (ref1CharIndex != (ui32)ref1.length());
        }
    }
    return ans;
}

TVector<TVector<ui32>> TWordwiseLevenshteinDistance::CalculateAllDistancesBackward(const TText& ref1, const TText& ref2) const {
    TVector<TVector<ui32>> dp(ref1.Size() + 1, TVector<ui32>(ref2.Size() + 1));
    for (int i = ref2.Size(); i >= 0; i--) {
        dp[ref1.Size()][i] = ref2.Size() - i;
    }
    for (int i = (int)ref1.Size() - 1; i >= 0; i--) {
        dp[i][ref2.Size()] = ref1.Size() - i;
        for (int j = (int)ref2.Size() - 1; j >= 0; j--) {
            dp[i][j] = std::min(dp[i + 1][j], dp[i][j + 1]) + 1;
            dp[i][j] = std::min(dp[i][j], dp[i + 1][j + 1] + (ref1.Words[i] != ref2.Words[j]));
        }
    }
    return dp;
}

double LikelihoodDistance(double pos1, double pos2) {
    return ADDITIONAL - (pos1 - pos2) * (pos1 - pos2) / (2 * SIGMA * SIGMA);
}

TVector<TVector<double>>
TLikelihoodWordwiseLevensteinDistance::CalculateAllDistancesBackward(const TText& ref1, const TText& ref2) const {
    TVector<TVector<double>> dp(
        ref1.Size() + 1,
        TVector<double>(ref2.Size() + 1));
    for (int i = ref2.Size(); i >= 0; i--) {
        dp[ref1.Size()][i] = ref2.Size() - i;
    }
    for (int i = (int)ref1.Size() - 1; i >= 0; i--) {
        dp[i][ref2.Size()] = ref1.Size() - i;
        for (int j = (int)ref2.Size() - 1; j >= 0; j--) {
            dp[i][j] = std::min(dp[i + 1][j], dp[i][j + 1]) + 1;

            double p = dp[i + 1][j + 1];
            if (ref1.Words[i] != ref2.Words[j]) {
                p += 1;
            } else {
                p -= Weight * LikelihoodDistance(ref1.GetOnset(i), ref2.GetOnset(j));
            }
            dp[i][j] = std::min(dp[i][j], p);
        }
    }
    return dp;
}

TVector<double> TLikelihoodWordwiseLevensteinDistance::RestoreLikelihoods(const TText& ref1, const TText& ref2) const {
    auto dp = CalculateAllDistancesBackward(ref1, ref2);
    TVector<double> ans;
    int i = 0;
    int j = 0;
    while (i < ref1.Size() && j < ref2.Size()) {
        double p = dp[i + 1][j + 1];
        if (ref1.Words[i] != ref2.Words[j]) {
            p += 1;
        } else {
            p -= Weight * LikelihoodDistance(ref1.GetOnset(i), ref2.GetOnset(j));
        }
        if (dp[i + 1][j] < dp[i][j + 1] && dp[i + 1][j] < p) {
            i++;
        } else if (dp[i][j + 1] < p) {
            j++;
        } else {
            ans.push_back(((double)ref1.GetLetterOnset(i) - (double)ref2.GetLetterOnset(j)) / SIGMA);
            i++;
            j++;
        }
    }
    return ans;
}

TVector<TVector<double>>
TLikelihoodLevensteinDistance::CalculateAllDistancesBackward(const TText& ref1_, const TText& ref2_) const {
    TString ref1 = ref1_.JoinWithWhitespaces();
    TString ref2 = ref2_.JoinWithWhitespaces();
    TVector<TVector<double>> dp(ref1.length() + 1, TVector<double>(ref2.length() + 1));
    for (int i = ref2.length(); i >= 0; i--) {
        dp[ref1.length()][i] = ref2.length() - i;
    }
    for (int i = (int)ref1.length() - 1; i >= 0; i--) {
        dp[i][ref2.length()] = ref1.length() - i;
        for (int j = (int)ref2.length() - 1; j >= 0; j--) {
            dp[i][j] = std::min(dp[i + 1][j], dp[i][j + 1]) + 1;

            double p = dp[i + 1][j + 1];
            if (ref1[i] != ref2[j]) {
                p += 1;
            } else {
                p += Weight * LikelihoodDistance(ref1_.GetLetterOnset(i), ref2_.GetLetterOnset(j));
            }
            dp[i][j] = std::min(dp[i][j], p);
        }
    }

    TVector<TVector<double>> ans(ref1_.Size() + 1, TVector<double>(ref2_.Size() + 1));
    ui32 ref1CharIndex = ref1.length();
    for (int i = ref1_.Size(); i >= 0; i--) {
        ui32 ref2CharIndex = ref2.length();
        for (int j = ref2_.Size(); j >= 0; j--) {
            ans[i][j] = dp[ref1CharIndex][ref2CharIndex];
            if (j > 0) {
                ref2CharIndex -= ref2_.Words[j - 1].length() + (ref2CharIndex != (ui32)ref2.length());
            }
        }
        if (i > 0) {
            ref1CharIndex -= ref1_.Words[i - 1].length() + (ref1CharIndex != (ui32)ref1.length());
        }
    }
    return ans;
}

TVector<double> TLikelihoodLevensteinDistance::RestoreLikelihoods(const TText& ref1_, const TText& ref2_) const {
    TString ref1 = ref1_.JoinWithWhitespaces();
    TString ref2 = ref2_.JoinWithWhitespaces();
    TVector<TVector<double>> dp(ref1.length() + 1, TVector<double>(ref2.length() + 1));
    for (int i = ref2.length(); i >= 0; i--) {
        dp[ref1.length()][i] = ref2.length() - i;
    }
    for (int i = (int)ref1.length() - 1; i >= 0; i--) {
        dp[i][ref2.length()] = ref1.length() - i;
        for (int j = (int)ref2.length() - 1; j >= 0; j--) {
            dp[i][j] = std::min(dp[i + 1][j], dp[i][j + 1]) + 1;

            double p = dp[i + 1][j + 1];
            if (ref1[i] != ref2[j]) {
                p += 1;
            } else {
                p -= Weight * LikelihoodDistance(ref1_.GetLetterOnset(i), ref2_.GetLetterOnset(j));
            }
            dp[i][j] = std::min(dp[i][j], p);
        }
    }
    TVector<double> ans;
    size_t i = 0;
    size_t j = 0;
    while (i < ref1.length() && j < ref2.length()) {
        double p = dp[i + 1][j + 1];
        if (ref1[i] != ref2[j]) {
            p += 1;
        } else {
            p -= Weight * LikelihoodDistance(ref1_.GetLetterOnset(i), ref2_.GetLetterOnset(j));
        }
        if (dp[i + 1][j] < dp[i][j + 1] && dp[i + 1][j] < p) {
            i++;
        } else if (dp[i][j + 1] < p) {
            j++;
        } else {
            if (ref1[i] == ref2[j]) {
                double x = (double)ref1_.GetLetterOnset(i) - (double)ref2_.GetLetterOnset(j);
                if (abs(x) > 2000) {
                    Cerr << "x is too large: " << x << Endl;
                }
                ans.push_back(x);
            }
            i++;
            j++;
        }
    }
    return ans;
}

TVector<TVector<TLikelihoodValue>>
TTupleLikelihoodLevensteinDistance::CalculateAllDistancesBackward(const TText& ref1_, const TText& ref2_) const {
    TString ref1 = ref1_.JoinWithWhitespaces();
    TString ref2 = ref2_.JoinWithWhitespaces();
    TVector<TVector<TLikelihoodValue>> dp(ref1.length() + 1, TVector<TLikelihoodValue>(ref2.length() + 1));
    for (int i = ref2.length(); i >= 0; i--) {
        dp[ref1.length()][i] = ref2.length() - i;
    }
    for (int i = (int)ref1.length() - 1; i >= 0; i--) {
        dp[i][ref2.length()] = ref1.length() - i;
        for (int j = (int)ref2.length() - 1; j >= 0; j--) {
            dp[i][j] = std::min(dp[i + 1][j], dp[i][j + 1]) + 1;

            TLikelihoodValue p = dp[i + 1][j + 1];
            if (ref1[i] != ref2[j]) {
                p.Value += 1;
            } else {
                p.Corresponding += 1;
                p.Likelihood -= Weight * LikelihoodDistance(ref1_.GetLetterOnset(i), ref2_.GetLetterOnset(j));
            }
            dp[i][j] = std::min(dp[i][j], p);
        }
    }

    TVector<TVector<TLikelihoodValue>> ans(ref1_.Size() + 1, TVector<TLikelihoodValue>(ref2_.Size() + 1));
    ui32 ref1CharIndex = ref1.length();
    for (int i = ref1_.Size(); i >= 0; i--) {
        ui32 ref2CharIndex = ref2.length();
        for (int j = ref2_.Size(); j >= 0; j--) {
            ans[i][j] = dp[ref1CharIndex][ref2CharIndex];
            if (j > 0) {
                ref2CharIndex -= ref2_.Words[j - 1].length() + (ref2CharIndex != (ui32)ref2.length());
            }
        }
        if (i > 0) {
            ref1CharIndex -= ref1_.Words[i - 1].length() + (ref1CharIndex != (ui32)ref1.length());
        }
    }
    return ans;
}

TVector<double> TTupleLikelihoodLevensteinDistance::RestoreLikelihoods(const TText& ref1_, const TText& ref2_) const {
    TString ref1 = ref1_.JoinWithWhitespaces();
    TString ref2 = ref2_.JoinWithWhitespaces();
    TVector<TVector<TLikelihoodValue>> dp(ref1.length() + 1, TVector<TLikelihoodValue>(ref2.length() + 1));
    for (int i = ref2.length(); i >= 0; i--) {
        dp[ref1.length()][i] = ref2.length() - i;
    }
    for (int i = (int)ref1.length() - 1; i >= 0; i--) {
        dp[i][ref2.length()] = ref1.length() - i;
        for (int j = (int)ref2.length() - 1; j >= 0; j--) {
            dp[i][j] = std::min(dp[i + 1][j], dp[i][j + 1]) + 1;

            TLikelihoodValue p = dp[i + 1][j + 1];
            if (ref1[i] != ref2[j]) {
                p.Value += 1;
            } else {
                p.Corresponding += 1;
                p.Likelihood -= Weight * LikelihoodDistance(ref1_.GetLetterOnset(i), ref2_.GetLetterOnset(j));
            }
            dp[i][j] = std::min(dp[i][j], p);
        }
    }
    TVector<double> ans;
    size_t i = 0;
    size_t j = 0;
    while (i < ref1.length() && j < ref2.length()) {
        TLikelihoodValue p = dp[i + 1][j + 1];
        if (ref1[i] != ref2[j]) {
            p.Value += 1;
        } else {
            p.Corresponding += 1;
            p.Likelihood -= Weight * LikelihoodDistance(ref1_.GetLetterOnset(i), ref2_.GetLetterOnset(j));
        }
        if (dp[i + 1][j] < dp[i][j + 1] && dp[i + 1][j] < p) {
            i++;
        } else if (dp[i][j + 1] < p) {
            j++;
        } else {
            double x = (double)ref1_.GetLetterOnset(i) - (double)ref2_.GetLetterOnset(j);
            if (abs(x) > 2000) {
                Cerr << "x is too large: " << x << Endl;
            }
            ans.push_back(x);
            i++;
            j++;
        }
    }
    return ans;
}

TVector<TVector<TLikelihoodValue>>
TTupleLikelihoodWordwiseLevensteinDistance::CalculateAllDistancesBackward(const TText& ref1, const TText& ref2) const {
    TVector<TVector<TLikelihoodValue>> dp(
        ref1.Size() + 1,
        TVector<TLikelihoodValue>(ref2.Size() + 1));
    for (int i = ref2.Size(); i >= 0; i--) {
        dp[ref1.Size()][i] = ref2.Size() - i;
    }
    for (int i = (int)ref1.Size() - 1; i >= 0; i--) {
        dp[i][ref2.Size()] = ref1.Size() - i;
        for (int j = (int)ref2.Size() - 1; j >= 0; j--) {
            dp[i][j] = std::min(dp[i + 1][j], dp[i][j + 1]) + 1;

            auto p = dp[i + 1][j + 1];
            if (ref1.Words[i] != ref2.Words[j]) {
                p.Value += 1;
            } else {
                p.Likelihood -= Weight * LikelihoodDistance(ref1.GetOnset(i), ref2.GetOnset(j));
            }
            dp[i][j] = std::min(dp[i][j], p);
        }
    }
    return dp;
}

TVector<double>
TTupleLikelihoodWordwiseLevensteinDistance::RestoreLikelihoods(const TText& ref1, const TText& ref2) const {
    auto dp = CalculateAllDistancesBackward(ref1, ref2);
    TVector<double> ans;
    int i = 0;
    int j = 0;
    while (i < ref1.Size() && j < ref2.Size()) {
        auto p = dp[i + 1][j + 1];
        if (ref1.Words[i] != ref2.Words[j]) {
            p.Value += 1;
        } else {
            p.Likelihood -= Weight * LikelihoodDistance(ref1.GetOnset(i), ref2.GetOnset(j));
        }
        if (dp[i + 1][j] < dp[i][j + 1] && dp[i + 1][j] < p) {
            i++;
        } else if (dp[i][j + 1] < p) {
            j++;
        } else {
            ans.push_back(((double)ref1.GetLetterOnset(i) - (double)ref2.GetLetterOnset(j)) / SIGMA);
            i++;
            j++;
        }
    }
    return ans;
}
