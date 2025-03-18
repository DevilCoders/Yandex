#include <library/cpp/getopt/opt.h>

#include <library/cpp/charset/ci_string.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/stream/zlib.h>
#include <util/string/cast.h>
#include <util/string/vector.h>

float frequencyThreshold = 0.00001f;
int   max_contexts         = -1;
bool  harvester_out       = false;

class TBadZipArchive {
private:
    TUnbufferedFileInput      input;
    TZLibDecompress zip;

    TVector<TString> tokens;

    char *buffer;
    size_t bSize;
public:
    TBadZipArchive(const char *filename, size_t buffer_size = 1024) : input(filename), zip(&input) {
        buffer = new char[buffer_size];
        bSize = buffer_size;
    }

    ~TBadZipArchive() {
        delete [] buffer;
    }

    bool ReadLine(TString &s) {
        FillBuffer();
        if(tokens.ysize() == 0) {
            return false;
        }

        TString TCiString(tokens.front());
        tokens.erase(tokens.begin());
        const char *ch;
        const char *str = TCiString.c_str();
        if((ch = strstr(str, "."))) {
            unsigned position = (unsigned)(ch - str + 1);
            for(unsigned i = position; i < TCiString.length(); i++) {
                if(str[i] < '0' || str[i] > '9') {
                    if(str[i] == 'e' && str[i+1] == '-') i++;
                    else {
                        s = TCiString.substr(0, i);
                        tokens.insert(tokens.begin(), TCiString.substr(i));
                        return true;
                    }
                }
            }
        }

        s = TCiString;
        return true;
    }

private:
    void FillBuffer() {
        if(tokens.ysize() > 1) return;
        size_t read;
        if((read = zip.Read(buffer, bSize)) > 0) {
            TString s(buffer, read);
            if(tokens.ysize() == 1) {
                TString ss = tokens.front();
                s = tokens.front() + s;
                tokens.erase(tokens.begin());
            }
            TVector<TString> vs = SplitString(s, " ", 0, KEEP_EMPTY_TOKENS);
            tokens.insert(tokens.end(), vs.begin(), vs.end());
        }
    }
};

class TGoodZipArchive {
private:
    TUnbufferedFileInput      input;
    TZLibDecompress zip;

    TString token;

    char *buffer;
    size_t bSize;
public:
    TGoodZipArchive(const char *filename, size_t buffer_size = 1024) : input(filename), zip(&input) {
        buffer = new char[buffer_size];
        bSize = buffer_size;
    }

    bool ReadLine(TString &s) {
        if(token.length() > 0) {
            s = token;
            token.clear();
            return true;
        }
        TString ss;
        if(!zip.ReadLine(ss)) return false;

        TVector<TString> vs = SplitString(ss, " ");
        if(ss.at(0) == ' ') {
            s = vs[0] + vs[1];
        } else {
            token = vs[1];
            s = vs[0];
        }
        return true;
    }
};

template<class T>
struct HPair {
    TString  token;
    T       number;

    HPair() {}

    HPair(TString _token, T _number) : token(_token), number(_number) {}
};

struct HBPair : public HPair<double> {
    bool    topLevel;
};

class TSheaf {
private:
    HPair<int> head;
    TMap<TString, double> contexts;
public:
    TSheaf() {}

    TSheaf(HPair<int> _head) : head(_head) {}

    void Add(HPair<double> p) {
        if(contexts.find(p.token) != contexts.end()) {
            contexts[p.token] += p.number;
        } else {
            contexts[p.token] = p.number;
        }
    }
private:
    void Cleanup() {
        TMap<TString, double> ctx;
        for(TMap<TString, double>::iterator i = contexts.begin(); i != contexts.end(); i++) {
            if(i->second / head.number >= frequencyThreshold) {
                ctx[i->first] = i->second / head.number;
            }
        }
        contexts.clear();
        contexts = ctx;
    }
public:
    TString getKey() {
        return head.token;
    }

    static void merge(TSheaf& result, TMap<int, TSheaf*> &sheafs) {
        result.head.number = 0;
        result.contexts.clear();
        for(TMap<int, TSheaf*>::iterator i = sheafs.begin(); i != sheafs.end(); i++) {
            if(i == sheafs.begin()) result.head.token = i->second->head.token;
            result.head.number += i->second->head.number;
            for(TMap<TString, double>::iterator j = i->second->contexts.begin(); j != i->second->contexts.end(); j++) {
                result.Add(
                    HPair<double>(
                        j->first,
                        !harvester_out ? j->second * i->second->head.number : j->second
                    )
                );
            }
        }
        result.Cleanup();
    }

    template<class T>
    friend class TSheafsReader;

    friend IOutputStream & operator << (IOutputStream & s, TSheaf &sh);
};

class TAbstractSheafsReader {
public:
    virtual ~TAbstractSheafsReader() {}
    virtual bool next(TSheaf &sheaf) = 0;
};

template<class T>
class TSheafsReader : private T, public TAbstractSheafsReader {
public:
    TSheafsReader(const char *filename, size_t buffer_size = 1024)
        : T(filename, buffer_size) {hP.topLevel = false;}
private:
    bool next(HBPair &p, bool x = false) {
        static TString s;
        const char *ch;
        if(T::ReadLine(s)) {
            if(!(ch = strstr(s.c_str(), "."))) {
                if(!x) {
                    p.token = s;
                    T::ReadLine(s);
                }
                p.number = FromString<int>(s);
                p.topLevel = true;
            } else {
                int position = (int)(ch - s.c_str());
                while(s.at(position - 1) >= '0' && s.at(position - 1) <= '9') position--;
                p.token = s.substr(0, position);
                p.number = FromString<double>(s.substr(position));
                p.topLevel = false;
            }
            return true;
        } else {
            return false;
        }
    }
private:
    HPair<int> hS;
    HBPair       hP;
public:
    bool next(TSheaf &sheaf) override {
        sheaf.contexts.clear();
        if(hS.token.length() > 0) {
            hP.topLevel = true;
            hP.token = hS.token;
            hS.token.clear();
        } else {
            if(!hP.topLevel) {
                if(!next(hP) || !hP.topLevel) return false;
            }
        }
        sheaf.head.token = hP.token;
        sheaf.head.number = int(hP.number);
        bool n = false;
        while((n = next(hP)) && !hP.topLevel) {
            sheaf.contexts[hP.token] = hP.number;
        }
        hS.token  = hP.token;
        hS.number = int(hP.number);
        return n;
    }
};

template<class T>
class TMultiSheafsReader : public TAbstractSheafsReader  {
private:
    TVector<TSheafsReader<T> *> readers;
    TVector<TSheaf> sheafs;
public:
    TMultiSheafsReader(TVector<TString> files) {
        for(TVector<TString>::iterator i = files.begin(); i != files.end(); i++) {
            TSheafsReader<T> *reader = new  TSheafsReader<T>((*i).c_str());
            readers.push_back(reader);
            TSheaf sheaf;
            reader->next(sheaf);

            sheafs.push_back(sheaf);
        }
    }

    bool next(TSheaf &sheaf) override {
        TString min = sheafs.front().getKey();
        for(TVector<TSheaf>::iterator i = sheafs.begin() + 1; i != sheafs.end(); i++) {
            if(min > (*i).getKey()) min = (*i).getKey();
        }

        TMap<int, TSheaf*> to_pass;
        int index = 0;
        for(TVector<TSheaf>::iterator i = sheafs.begin(); i != sheafs.end(); i++, index++) {
            if(min == (*i).getKey()) {
                to_pass[index] = i;
            }
        }

        TSheaf::merge(sheaf, to_pass);

        for(TMap<int, TSheaf*>::iterator i = to_pass.begin(); i != to_pass.end(); i++) {
            int index = i->first;
            if(!readers[index]->next(sheafs[index])) {
                sheafs.erase(sheafs.begin() + index);
                readers.erase(readers.begin() + index);
            }
        }

        return true;
    }
};

IOutputStream & operator << (IOutputStream & s, TSheaf &sh) {
    s << sh.head.token << " " << sh.head.number << Endl;
    int _i = 0; TMap<TString, double>::iterator p = sh.contexts.begin();
    while(_i != max_contexts && p != sh.contexts.end()) {
        s << " " << p->first << " " << p->second << Endl;
        _i++;
        p++;
    }
    int remains = sh.contexts.size() - _i;
    if(remains > 0) {
        s << " And " << remains << " more..." << Endl;
    }
    return s;
}


int _main(int argc, char* argv[]) {
    Opt opt(argc, argv, "f:m:zo:");
    int optcode = EOF;

    TString outname;

    while ((optcode = opt.Get()) != EOF) {
        switch (optcode) {
            case 'f':
                frequencyThreshold = FromString<float>(opt.Arg);
                break;
            case 'm':
                max_contexts = FromString<int>(opt.Arg);
                break;
            case 'z':
                harvester_out = true;
                break;
            case 'o':
                outname = TString(opt.Arg);
                break;
        }
    }

    TBuffered<TUnbufferedFileOutput> output(0x40000, outname);

    TVector<TString> files;
    int i = opt.Ind;
    Cerr << "Archives: " << Endl;
    while(i < argc) {
        TString file(argv[i]);
        files.emplace_back();
        Cerr << file << Endl;
        i++;
    }

    TSheaf sheaf;
    TAbstractSheafsReader *reader;
    if(!harvester_out) {
        reader = new TMultiSheafsReader<TBadZipArchive>(files);
    } else {
        reader = new TMultiSheafsReader<TGoodZipArchive>(files);
    }
    while(reader->next(sheaf)) {
        Cerr << sheaf.getKey() << Endl;
        output << sheaf;
    }

    delete reader;

    return 0;
}

int main(int argc, char* argv[]) {
#ifdef WIN32
#ifdef _DEBUG
    system("chcp 1251");
#endif
#endif
    int result = _main(argc, argv);
#ifdef WIN32
#ifdef _DEBUG
    system("pause");
#endif
#endif
    return result;
}
