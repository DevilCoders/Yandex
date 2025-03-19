#pragma once

#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/string/util.h>
#include <util/stream/file.h>

struct TWideToken;

// Collection, mapping word to it's forces
class TWordForces {
private:
    // Maps word to bits, i-th set bit says that i-th vowel is stressed
    typedef TMap<ui32, ui32> TMapType;
    TMapType Map;

    TWordForces(const TWordForces &);
    TWordForces &operator = (const TWordForces &);

public:
    TWordForces() {}
    ~TWordForces() {}
    // Read the vocabulary
    void Load(const char *file_name);
    // Get information about word's forces
    void GetForces(const TWideToken &tok, TVector<float>* res);
};


// Poetry rithms
enum TRithmType {
    RT_FIRST,

    RT_HOREY = RT_FIRST,
    RT_HOREY_M,
    RT_YAMB,
    RT_YAMB_M,
    RT_DACTIL,
    RT_AMPHIBRAHIY,
    RT_ANAPEST,

    RT_COUNT,

    RT_UNKNOWN,
    RT_ABSENT,
    RT_FEW_INFO,
    RT_ZONE_BREAK,
    RT_EMPTY
};


// Class for determine the rithm of a strophe
class TStrophePoetryMatcher {
private:
    TVector<float> Forces;
    float Rithms[RT_COUNT];
    size_t Counts[RT_COUNT];
    size_t WordCount;

public:
    TStrophePoetryMatcher()
        : WordCount(0)
    {
        std::fill(Rithms, Rithms+RT_COUNT, 0.0f);
        std::fill(Counts, Counts+RT_COUNT, 0);
    }
    void Clear() {
        Forces.clear();
        std::fill(Rithms, Rithms+RT_COUNT, 0.0f);
        std::fill(Counts, Counts+RT_COUNT, 0);
        WordCount = 0;
    }
    // Add information about word
    void AddWord(const TVector<float> &forces) {
        if (Forces.size()<=20)
            Forces.insert(Forces.end(), forces.begin(), forces.end());
        ++WordCount;
    }
    // Calc rithms' probabilities and suggest the most matched
    TRithmType Calculate();
    const float *GetRithms() const { return Rithms; }
    size_t GetWordCount() const { return WordCount; }

private:
    // Add vowel to all rithms
    void ProcessVowel(size_t v_num, float prob);
};


// Class for determine poetry value of the whole document
class TPoetryMatcher {
private:
    TStrophePoetryMatcher Strophe;
    TVector<std::pair<TRithmType, float> > Values;

public:
    // Add information about word
    void AddWord(const TVector<float> &forces) {
        Strophe.AddWord(forces);
    }
    // Go to the next line
    void FlushLine();
    // Mark zone break
    void OnZoneBreak();
    // Poetry value of the whole document
    float PoetryValue() const;
    // Poetry vaue of the max continuous 4 strophes
    float PoetryValue2() const;
};
