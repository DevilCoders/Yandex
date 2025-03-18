#include "main.h"

#include <library/cpp/charset/wide.h>
#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/deprecated/mapped_file/mapped_file.h>
#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/streams/factory/factory.h>

#include <util/folder/path.h>
#include <util/string/vector.h>
#include <util/system/filemap.h>

namespace {
    enum EValueTypes : size_t {
        VT_I16,
        VT_I32,
        VT_I64,
        VT_UI16,
        VT_UI32,
        VT_UI64,
        VT_BOOL,
        VT_FLOAT,
        VT_DOUBLE,
        VT_TSTRING,
        VT_WTROKA,
        VT_PAIR_OF_TSTRING,
        VT_TRIE_SET
    };
}

template <EValueTypes Index = VT_I16> struct TValueType { using value = i16; };
template <> struct TValueType<VT_I32> { using value = i32; };
template <> struct TValueType<VT_UI16> { using value = ui16; };
template <> struct TValueType<VT_I64> { using value = i64; };
template <> struct TValueType<VT_UI32> { using value = ui32; };
template <> struct TValueType<VT_UI64> { using value = ui64; };
template <> struct TValueType<VT_BOOL> { using value = bool; };
template <> struct TValueType<VT_FLOAT> { using value = float; };
template <> struct TValueType<VT_DOUBLE> { using value = double; };
template <> struct TValueType<VT_TSTRING> { using value = TString; };
template <> struct TValueType<VT_WTROKA> { using value = TUtf16String; };
template <> struct TValueType<VT_PAIR_OF_TSTRING> { using value = std::pair<TString, TString>; };
template <> struct TValueType<VT_TRIE_SET> { using value = char; };

namespace {
    struct TArgs {
        bool Wide = false;
        bool Array = false;
        TString Type;
        ECharset Encoding = CODES_UNKNOWN;
        TFsPath TriePath;
        TFsPath OutputPath;
        bool UseAsIsParker = false;
    };
}  // namespace

static TArgs ParseOptions(const int argc, const char* argv[]) {
    auto opts = NLastGetopt::TOpts::Default();
    TArgs args;
    opts.AddLongOption('w', "wide", "Trie is made of wide characters")
        .Optional()
        .NoArgument()
        .SetFlag(&args.Wide);
    opts.AddLongOption('e', "encoding", "Output encoding (used only with -w)")
        .Optional()
        .DefaultValue("UTF-8")
        .StoreMappedResultT<TString>(&args.Encoding, [](const TString& value) {
            const auto encoding = CharsetByName(value.c_str());
            Y_ENSURE(CODES_UNKNOWN != encoding, "Unknown encoding");
            return encoding;
        });
    opts.AddLongOption('a', "array", "Trie values are arrays")
        .Optional()
        .NoArgument()
        .SetFlag(&args.Array);
    opts.AddLongOption('t', "type", "Type of value or array element")
        .Optional()
        .DefaultValue("ui64")
        .Help("Type of value or array element.\n"
              "Possible values are: ui32, i32, ui64, i64, bool, float, double, TString, TUtf16String, PairOfTString"
              " 0 (for TrieSet)")
        .StoreResult(&args.Type);
    opts.AddLongOption('P', "as-is-packer", "Use as is packer")
        .NoArgument()
        .SetFlag(&args.UseAsIsParker);
    opts.AddLongOption('o', "output")
        .DefaultValue("-")
        .StoreResult(&args.OutputPath)
        .Help("Output file");
    opts.SetFreeArgsNum(1);
    opts.SetFreeArgTitle(0, "<input file>", "file with trie");
    NLastGetopt::TOptsParseResult parseResult{&opts, argc, argv};
    args.TriePath = parseResult.GetFreeArgs().front();
    return args;
}

template <typename TValue>
static void ProcessWideTrieWithArray(const TMappedFile& mf, const ECharset enc, IOutputStream& out) {
    const TCompactTrie<TChar, TVector<TValue>> trie(static_cast<const char*>(mf.getData()),
                                                    mf.getSize());
    for (const auto& value : trie) {
        const TString key = WideToChar(value.first, enc);
        TVector<TString> values(value.second.size());
        for (size_t i = 0; i < value.second.size(); ++i) {
            values[i] = ToString(value.second[i]);
        }
        out << key << '\t' << JoinStrings(values, "\t") << '\n';
    }
}

template <>
void ProcessWideTrieWithArray<TUtf16String>(const TMappedFile& mf, const ECharset enc,
                                      IOutputStream& out) {
    const TCompactTrie<TChar, TVector<TUtf16String>> trie(static_cast<const char*>(mf.getData()),
                                                    mf.getSize());
    for (const auto& value : trie) {
        const TString key = WideToChar(value.first, enc);
        TVector<TString> values(value.second.size());
        for (size_t i = 0; i < value.second.size(); ++i) {
            values[i] = WideToChar(value.second[i], enc);
        }
        out << key << '\t' << JoinStrings(values, "\t") << '\n';
    }
}

template <typename TValue>
static void ProcessTrieWithArray(const TMappedFile& mf, IOutputStream& out) {
    const TCompactTrie<char, TVector<TValue>> trie(static_cast<const char*>(mf.getData()),
                                                   mf.getSize());
    for (const auto& value: trie) {
        TVector<TString> values(value.second.size());
        for (size_t i = 0; i < value.second.size(); ++i) {
            values[i] = ToString(value.second[i]);
        }
        out << value.first << '\t' << JoinStrings(values, "\t") << '\n';
    }
}

template <>
void ProcessTrieWithArray<TUtf16String>(const TMappedFile& mf, IOutputStream& out) {
    const TCompactTrie<char, TVector<TUtf16String>> trie(static_cast<const char*>(mf.getData()),
                                                   mf.getSize());
    for (const auto& value: trie) {
        TVector<TString> values(value.second.size());
        for (size_t i = 0; i < value.second.size(); ++i) {
            values[i] = WideToUTF8(value.second[i]);
        }
        out << value.first << '\t' << JoinStrings(values, "\t") << '\n';
    }
}

template <typename TValue, typename TPacker = TCompactTriePacker<TValue>>
static void ProcessWideTrie(const TMappedFile& mf, const ECharset enc, IOutputStream& out) {
    const TCompactTrie<TChar, TValue, TPacker> trie(static_cast<const char*>(mf.getData()), mf.getSize());
    for (const auto& value : trie) {
        const TString key = WideToChar(value.first, enc);
        out << key << "\t" << value.second << '\n';
    }
}

template <typename TValue, typename TPacker = TCompactTriePacker<TValue>>
static void ProcessTrie(const TMappedFile& mf, IOutputStream& out) {
    const TCompactTrie<char, TValue, TPacker> trie(static_cast<const char*>(mf.getData()), mf.getSize());
    for (const auto& value : trie) {
        out << value.first << '\t' << value.second << '\n';
    }
}

static void ProcessWideTrieSet(const TMappedFile& mf, const ECharset enc, IOutputStream& out) {
    const TCompactTrie<TChar, ui32, TNullPacker<ui32>> trie(static_cast<const char*>(mf.getData()),
                                                            mf.getSize());
    for (const auto& value : trie) {
        const TString key = WideToChar(value.first, enc);
        out << key << '\n';
    }
}

static void ProcessTrieSet(const TMappedFile& mf, IOutputStream& out) {
    const TCompactTrie<char, ui32, TNullPacker<ui32>> trie(static_cast<const char*>(mf.getData()),
                                                           mf.getSize());
    for (const auto& value : trie) {
        out << value.first << '\n';
    }
}

template <EValueTypes Type>
static void Process(const TArgs& args, IOutputStream& output) {
    TMappedFile mf(args.TriePath);
    if (args.Array) {
        if (args.Wide) {
            ProcessWideTrieWithArray<typename TValueType<Type>::value>(mf, args.Encoding, output);
        } else {
            ProcessTrieWithArray<typename TValueType<Type>::value>(mf, output);
        }
    } else {
        if (args.Wide) {
            if (Type == VT_TRIE_SET) {
                ProcessWideTrieSet(mf, args.Encoding, output);
            } else {
                if (args.UseAsIsParker) {
                    ProcessWideTrie<typename TValueType<Type>::value, TAsIsPacker<typename TValueType<Type>::value>>(mf, args.Encoding, output);
                } else {
                    ProcessWideTrie<typename TValueType<Type>::value>(mf, args.Encoding, output);
                }
            }
        } else {
            if (Type == VT_TRIE_SET) {
                ProcessTrieSet(mf, output);
            } else {
                if (args.UseAsIsParker) {
                    ProcessTrie<typename TValueType<Type>::value, TAsIsPacker<typename TValueType<Type>::value>>(mf, output);
                } else {
                    ProcessTrie<typename TValueType<Type>::value>(mf, output);
                }
            }
        }
    }
}

static int Main(const TArgs& args) {
    const auto output = OpenOutput(args.OutputPath);
    if (args.Type == "i16") {
        Process<VT_I16>(args, *output);
    } else if (args.Type == "ui16") {
        Process<VT_UI16>(args, *output);
    } else if (args.Type == "i32")
        Process<VT_I32>(args, *output);
    else if (args.Type == "i64")
        Process<VT_I64>(args, *output);
    else if (args.Type == "ui32")
        Process<VT_UI32>(args, *output);
    else if (args.Type == "ui64")
        Process<VT_UI64>(args, *output);
    else if (args.Type == "bool")
        Process<VT_BOOL>(args, *output);
    else if (args.Type == "float")
        Process<VT_FLOAT>(args, *output);
    else if (args.Type == "double")
        Process<VT_DOUBLE>(args, *output);
    else if (args.Type == "TString")
        Process<VT_TSTRING>(args, *output);
    else if (args.Type == "TUtf16String")
        Process<VT_WTROKA>(args, *output);
    else if (args.Type == "PairOfTString")
        Process<VT_PAIR_OF_TSTRING>(args, *output);
    else if (args.Type == "0")
        Process<VT_TRIE_SET>(args, *output);
    else {
        ythrow yexception() << "Unknown type: " << args.Type;
    }
    return EXIT_SUCCESS;
}

IOutputStream& operator << (IOutputStream& out, const std::pair<TString, TString>& value) {
    return out << value.first << "\t" << value.second << "\n";
}

static int Main(const int argc, const char* argv[]) {
    const auto args = ParseOptions(argc, argv);
    return Main(args);
}

int NTrieOps::MainPrint(const int argc, const char* argv[]) {
    return ::Main(argc, argv);
}
