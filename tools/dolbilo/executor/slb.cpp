#include "slb.h"

#include <util/stream/output.h>
#include <util/stream/input.h>
#include <util/random/fast.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/datetime/base.h>

class TFakeTranslatedHost: public ISlb::ITranslatedHost {
    public:
        inline TFakeTranslatedHost(const TString& host, const TString& source)
            : Host_(host)
            , Source_(source)
        {
        }

        ~TFakeTranslatedHost() override {
        }

        const TString& Result() override {
            return Host_;
        }

        const TString& Source() override {
            return Source_;
        }

        void OnBadHost() override {
        }

    private:
        TString Host_;
        TString Source_;
};

class TSimpleSlb::TImpl {
        typedef TSimpleSlb::ITranslatedHost ITranslatedHost;

        enum ETokenType {
            OPENBR = 0,
            CLOSEBR = 1,
            STRING = 2,
            EQUAL = 3,
            MCOMMA = 4,
            MEOF = 5
        };

        struct TToken {
            inline TToken(ETokenType t, const TString& v = TString())
                : type(t)
                , value(v)
            {
                //Cerr << (int)t << " " << v << Endl;
            }

            inline bool Eof() const noexcept {
                return type == MEOF;
            }

            ETokenType type;
            TString value;
        };

        class TTokenizer {
            public:
                inline TTokenizer(IInputStream* in)
                    : Input_(in)
                    , Saved_(EOF)
                    , Line_(0)
                {
                }

                inline ~TTokenizer() {
                }

                inline TToken Next() {
                    SkipWS();

                    char ch;

                    if (!Read(&ch)) {
                        return TToken(MEOF);
                    }

                    switch (ch) {
                        case '=':
                            return TToken(EQUAL);

                        case '{':
                            return TToken(OPENBR);

                        case '}':
                            return TToken(CLOSEBR);

                        case ',':
                            return TToken(MCOMMA);

                        default:
                            return TToken(STRING, TString(ch) + ReadString());
                    }

                    return TToken(MEOF);
                }

                inline TString Expect(ETokenType type, TToken& cur) {
                    if (cur.type == type) {
                        TString ret = cur.value;

                        cur = Next();

                        return ret;
                    }

                    ythrow yexception() << "format error(at line " <<  (unsigned)Line_ << ")";
                }

            private:
                inline void SkipWS() {
                    char ch;

                    while (Read(&ch)) {
                        if (!isspace(ch)) {
                            Back(ch);

                            return;
                        }
                    }
                }

                inline bool Read(char* ch) {
                    if (Saved_ != EOF) {
                        *ch = Saved_;
                        Saved_ = EOF;

                        return true;
                    }

                    if (Input_->Read(ch, 1)) {
                        if (*ch == '\n') {
                            ++Line_;
                        }

                        return true;
                    }

                    return false;
                }

                inline void Back(char ch) {
                    assert(Saved_ == EOF);

                    Saved_ = ch;
                }

                inline TString ReadString() {
                    TString ret;
                    char ch;

                    while (Read(&ch)) {
                        if (isspace(ch) || ch == '=' || ch == '{' || ch == '}' || ch == ',') {
                            Back(ch);

                            return ret;
                        }

                        ret += ch;
                    }

                    return ret;
                }

            private:
                IInputStream* Input_;
                int Saved_;
                size_t Line_;
        };

        class THostRec {
            public:
                inline THostRec(const TString& host = TString())
                    : Host_(host)
                    , NextTry_(0)
                {
                }

                inline ~THostRec() {
                }

                inline const TString& Host() const noexcept {
                    return Host_;
                }

                inline bool IsBad() const noexcept {
                    return MicroSeconds() < NextTry_;
                }

                inline void SetBad() {
                    NextTry_ = MicroSeconds() + 10000000;
                }

            private:
                TString Host_;
                ui64 NextTry_;
        };

        class THostTable: public TSimpleRefCount<THostTable> {
            public:
                inline THostTable(TTokenizer& toker, TToken& cur) {
                    while (cur.type == STRING) {
                        Hosts_.push_back(toker.Expect(STRING, cur));
                    }

                    if (Hosts_.empty()) {
                        ythrow yexception() << "empty host table";
                    }
                }

                inline THostRec* Next(long r) {
                    THostRec* host = nullptr;
                    long num = 0;

                    do {
                        host = &Hosts_[(r + num) % Hosts_.size()];
                        ++num;
                    } while (host->IsBad() && num < (long)Hosts_.size());

                    return host;
                }

            private:
                TVector<THostRec> Hosts_;
        };

        typedef TIntrusivePtr<THostTable> THostTableRef;
        typedef THashMap<TString, THostTableRef> THostTables;

        class TTranslatedHost: public ITranslatedHost {
            public:
                inline TTranslatedHost(THostRec* rec, const TString& source)
                    : Rec_(rec)
                    , Source_(source)
                {
                }

                ~TTranslatedHost() override {
                }

                const TString& Result() override {
                    return Rec_->Host();
                }

                const TString& Source() override {
                    return Source_;
                }

                void OnBadHost() override {
                    Rec_->SetBad();
                }

            private:
                THostRec* Rec_;
                TString Source_;
        };

    public:
        inline TImpl(IInputStream* input)
            : Rand_(42)
        {
            TTokenizer toker(input);

            TToken cur = toker.Next();

            while (!cur.Eof()) {
                TVector<TString> hosts;

                while (true) {
                    hosts.push_back(toker.Expect(STRING, cur));

                    if (cur.type != MCOMMA) {
                        break;
                    }

                    cur = toker.Next();
                }

                toker.Expect(EQUAL, cur);
                toker.Expect(OPENBR, cur);

                THostTableRef tbl(new THostTable(toker, cur));

                toker.Expect(CLOSEBR, cur);

                for (TVector<TString>::const_iterator it = hosts.begin(); it != hosts.end(); ++it) {
                    Tables_[*it] = tbl;
                }
            }
        }

        inline ~TImpl() {
        }

        inline ITranslatedHost* Translate(const TString& host) {
            THostTables::iterator it = Tables_.find(host);

            if (it != Tables_.end()) {
                return new TTranslatedHost(it->second->Next(Rand_.GenRand()), host);
            }

            return new TFakeTranslatedHost(host, host);
        }

    private:
        THostTables Tables_;
        TReallyFastRng32 Rand_;
};

TSimpleSlb::TSimpleSlb(IInputStream* input)
    : Impl_(new TImpl(input))
{
}

TSimpleSlb::~TSimpleSlb() {
}

ISlb::TTranslatedHostRef TSimpleSlb::Translate(const TString& host) {
    return Impl_->Translate(host);
}

ISlb::TTranslatedHostRef TFakeSlb::Translate(const TString& host) {
    return new TFakeTranslatedHost(host, host);
}

ISlb::TTranslatedHostRef TReplaceSlb::Translate(const TString& host) {
    return new TFakeTranslatedHost(Replace_, host);
}
