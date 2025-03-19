#pragma once

#include <kernel/indexer/direct_text/dt.h>
#include <util/string/vector.h> // for TVector<TString>

// interface for detectors
class IWordDetector {
public:
    virtual void Prepare() {};
    virtual void Detect(ui32 /*position*/, const TString& /*lemma*/) = 0;
    virtual bool GetResult() = 0;
    virtual ~IWordDetector() {};
};

// simple detector (find only lemma)
class TSimpleDetector : public IWordDetector {
protected:
    bool WordIsFound;

public:
    TSimpleDetector() {
        Prepare();
    }

    void Prepare() override {
        WordIsFound = false;
    }

    void Detect(ui32 /*position*/, const TString& /*lemma*/) override {
        WordIsFound = true;
    }

    bool GetResult() override {
        return WordIsFound;
    }

};

// check words(lemms) in title
class TTitleDetector : public TSimpleDetector {
public:

    void Detect(ui32 position, const TString& lemma) override;
};

// catch one word and its position
class TPositionDetector : public IWordDetector {
private:
    ui32 WordPos;
    ui32 Distance;

protected:
    TPositionDetector(const TPositionDetector* prevDetector, ui32 distance)
        : WordPos(UndefinedPos)
        , Distance(distance)
        , PrevDetector(prevDetector)
        , IsLast(false)
    { }

    bool CurPositionIsCatched(ui32 curWordPos);

    const TPositionDetector* PrevDetector;
    bool IsLast;

public:
    static const ui32 UndefinedPos = (ui32)-1;

    class TFactory {
    public:
        TPositionDetector* CreateDetector(TPositionDetector* prevDetector = nullptr, ui32 distance = 1) {
            return new TPositionDetector(prevDetector, distance);
        }
    };

    void Prepare() override { // not used here
        WordPos = UndefinedPos;
    }

    void Detect(ui32 position, const TString& /*lemma*/) override;

    bool GetResult() override {
        return (WordPos != UndefinedPos);
    }

    inline ui32 GetCatchedPos() const {
        return WordPos;
    }

    void SetLast() {
        IsLast = true;
    }

};

