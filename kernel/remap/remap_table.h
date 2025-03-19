#pragma once

#include <util/system/defaults.h>
#include <util/generic/cast.h>
#include <util/generic/fwd.h>
#include <util/generic/vector.h>

class TRemapTable {
private:
    TVector<float> OriginalCP;
    TVector<i32> CPint;
    TVector<float> ScaleData;

    int Power;
public:
    TRemapTable() = default;
    TRemapTable(const float *pCP, int nCPcount) {
        OriginalCP.resize(nCPcount);
        for (int i = 0; i < nCPcount; ++i)
            OriginalCP[i] = pCP[i];

        Power = 0;
        while ((nCPcount + 1) > (1 << Power))
            ++Power;
        int elemCount = 1 << Power;

        TVector<float> CP;
        CP.resize(elemCount, pCP[nCPcount - 1]);
        CP[0] = -1e20f; // not uses actually
        for (int i = 0; i < nCPcount; ++i)
            CP[i + 1] = pCP[i];

        ScaleData.resize(elemCount * 2);
        for (int z = 0; z < nCPcount; ++z) {
            if (z == 0) {
                ScaleData[2 * z] = 0; // add
                ScaleData[2 * z + 1] = 0; // mult
            } else {
                float alpha = 1.0f / (nCPcount - 1);
                float cpDif1 = 0.f;
                if (CP[z + 1] != CP[z]) {
                    cpDif1 = 1.0f / (CP[z + 1] - CP[z]);
                }
                ScaleData[2 * z] = (z - 1 - CP[z] * cpDif1) * alpha; // add
                ScaleData[2 * z + 1] = cpDif1 * alpha; // mult
            }
        }
        for (int z = nCPcount; z < elemCount; ++z) {
            ScaleData[2 * z] = 1; // add
            ScaleData[2 * z + 1] = 0; // mult
        }

        // we use hack with replacing float point compare with integer one
        // in this case comparison results for negative floats are reversed, so we invert bits for negative numbers
        CPint.resize(elemCount);
        for (int k = 0; k < elemCount; ++k) {
            i32 val = BitCast<i32>(CP[k]);
            val ^= (val >> 31) & 0x7fffffff;
            CPint[k] = val;
        }
    }
    typedef union {
       float f;
       i32   i;
    } float2int;
    inline float Remap(float f) const {
        int nCP = 0;
        {
            float2int  fi;
            fi.f = f;
            int val  = fi.i;
            val ^= (val >> 31) &0x7fffffff;
            for (int i = Power - 1; i >= 0; --i) {
                int gr = (int)(val > CPint[nCP + (1 << i)]);
                nCP += gr << i;
            }
        }
        return f * ScaleData[nCP * 2 + 1] + ScaleData[nCP * 2];
    }
    float RemapBack(float f)
    {
        int nCPcount = OriginalCP.ysize();
        if (f < 0) {
            f = 0;
        }
        f *= (nCPcount - 1);
        int cp = (int)(f);
        float frac = f - cp;
        if (cp >= nCPcount - 1) {
            cp = nCPcount - 2;
            frac = 1;
        }
        return OriginalCP[cp] * (1 - frac) + OriginalCP[cp + 1] * frac;
    }
    const TVector<float>& GetOriginalCP() const {
        return OriginalCP;
    }
};

TString Encode(const TRemapTable &f);
bool Decode(TRemapTable *pRes, const TString &sz);
