#include "main.h"

#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/containers/comptrie/set.h>
#include <library/cpp/getopt/small/opt.h>

#include <library/cpp/charset/recyr.hh>
#include <util/charset/wide.h>
#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/util.h>

#include <cerrno>
#include <fstream>
#include <iostream>
#include <string>

#ifdef WIN32
#include <crtdbg.h>
#include <windows.h>
#endif // WIN32

namespace {

enum EValueTypes {
    VT_UI64,
    VT_FLOAT,
    VT_STRING,
    VT_WTROKA
};

struct TOptions {
    bool Silent;
    bool Timed;
    bool List;
    bool IsTrieSet;
    bool Wide;
    bool Array;
    EValueTypes Type;

    TString TrieFile;
    TString InputFile;
    TString OutputFile;

    TOptions()
        : Silent(false)
        , Timed(false)
        , List(false)
        , IsTrieSet(false)
        , Wide(false)
        , Array(false)
        , Type(VT_UI64)
    {}
};

//------------------------------------------

void usage(const char* progname) {
    std::string shortname = progname;
    size_t pos = (size_t)shortname.find_last_of("\\/:");
    if (pos != std::string::npos)
        pos = (size_t)shortname.find_last_not_of("\\/:", pos);
    if (pos != std::string::npos)
        shortname = shortname.substr(pos);

    std::cerr << "Usage: " << shortname << " [options] <trie file> [<input file>]" << std::endl;
    std::cerr << "Filters input, leaving only lines contained in a trie and adding leaf values" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "    -f          values are floats or arrays thereof" << std::endl;
    std::cerr << "    -W          values are TUtf16String or arrays thereof" << std::endl;
    std::cerr << "    -S          values are TString or arrays thereof" << std::endl;
    std::cerr << "    -a          values are arrays" << std::endl;
    std::cerr << "    -h, -?      Print this synopsis and exit" << std::endl;
    std::cerr << "    -o <file>   Output file" << std::endl;
    std::cerr << "    -l          List trie contents and exit (ignores input, -s and -t options)" << std::endl;
    std::cerr << "    -s          Silent mode - don't produce output (for performance metering)" << std::endl;
    std::cerr << "    -t          Print processing time  (for performance metering)" << std::endl;
    std::cerr << "    -w          Treat input keys as UTF-8, recode to TChar (wchar16)" << std::endl;
    std::cerr << "    -T          trie is actually trieset" << std::endl;
    std::cerr << "If no input file is specified, data is read from stdin." << std::endl;
    std::cerr << "If no output file is specified, results are sent to stdout." << std::endl;
}

template<typename TCharType>
struct TKeyReader
{};

template<>
struct TKeyReader<TChar> {
    typedef TUtf16String TKeyType;
    TUtf16String operator()(const char* s, size_t len) {
        return UTF8ToWide(s, len);
    }
};

template<>
struct TKeyReader<char> {
    typedef TStringBuf TKeyType;
    TStringBuf operator()(const char* start, size_t len) {
        return TStringBuf(start, len);
    }
};

class TTimer {
private:
    struct timeval Start;
    size_t LineCount;
public:
    TTimer() {
        gettimeofday(&Start, nullptr);
    }

    ~TTimer() {
        struct timeval end;
        gettimeofday(&end, nullptr);
        long millisecs = 1000 * (end.tv_sec - Start.tv_sec) + (end.tv_usec - Start.tv_usec) / 1000;
        Cerr << LineCount << " took " << millisecs << " ms to process" << Endl;
    }

    void SetLineCount(size_t lc) {
        LineCount = lc;
    }
};

template <typename T>
static IOutputStream& Print(IOutputStream& out, char sep, const T& value) {
    return out << sep << value;
}

template <typename T>
static IOutputStream& Print(IOutputStream& out, char sep, const TVector<T>& value) {
    for (const auto& it : value) {
        Print(out, sep, it);
    }
    return out;
}

template <class TTrie>
void List(const TTrie& trie, IOutputStream* out) {
    const bool doNotPrintValues = std::is_same< typename TTrie::TPacker, TNullPacker<typename TTrie::TData> >::value;
    for (typename TTrie::TConstIterator it = trie.Begin(), end = trie.End(); it != end; ++it) {
        if (doNotPrintValues)
            (*out) << it.GetKey() << '\n';
        else
            Print((*out) << it.GetKey(), '\t', it.GetValue()) << '\n';
    }
}

template <class TTrie>
size_t Filter(const TTrie& trie, IInputStream* input, IOutputStream* out, bool silent) {
    typedef TKeyReader<typename TTrie::TSymbol> TUsedKeyReader;
    TUsedKeyReader keyReader;
    typename TUsedKeyReader::TKeyType key;
    typename TTrie::TData res;
    TString line;
    size_t linecount = 0;
    while (input->ReadLine(line)) {
        key = keyReader(line.data(), line.size());
        ++linecount;
        if (trie.Find(key.data(), key.size(), &res)) {
            if (!silent)
                Print((*out) << line, ' ', res) << Endl;
        }
    }
    return linecount;
}

template <class TFileStream, class TStream>
TStream* InitStream(THolder<TFileStream>& holder, const TString& filename, TStream* defaultStream) {
    if (!filename.empty())
        holder.Reset(new TFileStream(filename));
    return holder.Get() ? holder.Get() : defaultStream;
}

template <class TTrie>
void RunOperation(TOptions& opt) {
    TTrie trie(TBlob::FromFile(opt.TrieFile));

    THolder<TOFStream> outputHolder;
    IOutputStream* output = InitStream(outputHolder, opt.OutputFile, &Cout);

    THolder<TTimer> timerHolder;
    if (opt.Timed)
        timerHolder.Reset(new TTimer);

    size_t lineCount = 0;
    if (opt.List) {
        List<TTrie>(trie, output);
    } else {
        THolder<TIFStream> inputHolder;
        IInputStream* input = InitStream(inputHolder, opt.InputFile, &Cin);
        lineCount = Filter<TTrie>(trie, input, output, opt.Silent);
    }

    if (opt.Timed)
        timerHolder->SetLineCount(lineCount);
}

template <bool array, typename T>
using TWrap = std::conditional_t<array, TVector<T>, T>;

template <typename TCharType, bool array = false>
void SelectDataType(TOptions& opt) {
    if (opt.IsTrieSet)
        RunOperation< TCompactTrieSet<TCharType> >(opt);
    else {
        if (!array && opt.Array) {
            SelectDataType<TCharType, true>(opt);
            return;
        }

        switch (opt.Type) {
            case VT_FLOAT:
                RunOperation< TCompactTrie<TCharType, TWrap<array, float>> >(opt);
                break;
            case VT_STRING:
                RunOperation< TCompactTrie<TCharType, TWrap<array, TString>> >(opt);
                break;
            case VT_WTROKA:
                RunOperation< TCompactTrie<TCharType, TWrap<array, TUtf16String>> >(opt);
                break;
            default:
                RunOperation< TCompactTrie<TCharType, TWrap<array, ui64>> >(opt);
        }
    }
}

void Execute(TOptions& opt) {
    if (opt.Wide)
        SelectDataType<TChar>(opt);
    else
        SelectDataType<char>(opt);
}

int Main(const int argc, const char* argv[]) {
#ifdef WIN32
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    ::SetConsoleCP(1251);
    ::SetConsoleOutputCP(1251);
#endif // WIN32

    Opt opt(argc, argv, "wfWSahlo:stT");
    int optcode = EOF;

    TOptions options;

    while ((optcode = opt.Get()) != EOF) {
        switch (optcode) {
        case '?':
        case 'h':
            usage(argv[0]);
            return 0;
        case 'f':
            options.Type = VT_FLOAT;
            break;
        case 'W':
            options.Type = VT_WTROKA;
            break;
        case 'S':
            options.Type = VT_STRING;
            break;
        case 'l':
            options.List = true;
            break;
        case 'o':
            options.OutputFile = opt.Arg;
            break;
        case 's':
            options.Silent = true;
            break;
        case 't':
            options.Timed = true;
            break;
        case 'T':
            options.IsTrieSet = true;
            break;
        case 'w':
            options.Wide = true;
            break;
        case 'a':
            options.Array = true;
            break;
        }
    }

    if (opt.Ind >= argc) {
        std::cerr << "Error - no trie file specified!" << std::endl;
        usage(argv[0]);
        return 1;
    }

    options.TrieFile = argv[opt.Ind];
    if (opt.Ind + 1 < argc)
        options.InputFile = argv[opt.Ind + 1];

    Execute(options);
    return EXIT_SUCCESS;
}
}

int NTrieOps::MainTest(const int argc, const char* argv[]) {
    return ::Main(argc, argv);
}
