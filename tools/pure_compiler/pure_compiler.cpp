#include <ysite/yandex/pure/constants.h>
#include <ysite/yandex/pure/grouped_by_word_pure_record_impl.h>
#include <ysite/yandex/pure/pure.h>
#include <ysite/yandex/pure/pure_header.h>
#include <ysite/yandex/pure/pure_internals.h>
#include <ysite/yandex/pure/pure_record_packer.h>

#include <library/cpp/containers/comptrie/comptrie_builder.h>
#include <library/cpp/getopt/last_getopt.h>
#include <kernel/lemmer/core/language.h>
#include <library/cpp/langmask/langmask.h>

#include <library/cpp/charset/doccodes.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/charset/wide.h>
#include <util/datetime/cputimer.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/vector.h>
#include <util/system/filemap.h>
#include <util/string/split.h>

struct TConfig {
    bool Verbose = false;
    bool Languages = false;
    bool UseFastLayout = false;
    ECharset Encoding = CODES_UTF8;
    bool DumpProtoHeader = false;
    bool DumpHeader = false;
    bool DumpOnlyForms = false;
    ELanguage DumpLanguage = LANG_UNK;
    enum class EFormat {OLD_READ, OLD_WRITE, NEW} Format = EFormat::OLD_READ;
};

//-----------------------------------------------

namespace {
class TPureLine {
public:
    class TBadLine: public yexception {
    };

    TUtf16String Key;
    ELanguage Lang = LANG_UNK;
    bool IsForm = false;
    NPure::ECase Case = NPure::LowCase;
    ui64 Value = 0;

private:
    void ParseFromOldRead(const TString& in, const TConfig& config) {
        TVector<TString> fields;
        StringSplitter(in).Split('\t').SkipEmpty().Collect(&fields);
        Y_VERIFY(fields.size() == (config.Languages ? 4 : 3));

        Key = CharToWide(fields[0], config.Encoding);
        int flags;
        if (config.Languages) {
            Lang = LanguageByNameStrict(fields[1]);
            flags = FromString<int>(fields[2]);
            Value = FromString<ui64>(fields[3]);
        }
        else {
            flags = FromString<int>(fields[1]);
            Value = FromString<ui64>(fields[2]);
        }

        Case = (flags & 1) ? NPure::TitleCase : NPure::LowCase;
        IsForm = flags & 2;
    }

    void ParseFromOldWrite(TStringBuf in) {
        size_t langDelimPos = in.find('|');
        if (langDelimPos != in.npos && in[0] == 'L') {
            Lang = LanguageByNameStrict(in.substr(1, langDelimPos - 2));
            in = in.substr(langDelimPos + 1);
        }
        if (in[0] == '!') {
            IsForm = true;
            in = in.substr(1);
        }
        size_t spacePos = in.find('\t');
        Y_ASSERT(spacePos != in.npos);
        if (in[spacePos - 1] == '\x01') {
            Case = NPure::TitleCase;
            Key = UTF8ToWide(in.substr(0, spacePos - 1));
        }
        else {
            Key = UTF8ToWide(in.substr(0, spacePos));
        }
        Value = FromString<ui64>(in.substr(spacePos + 1));
    }

    void ParseFromNew(const TString& in) {
        TVector<TString> fields;
        StringSplitter(in).Split('\t').Collect(&fields);
        Y_VERIFY(fields.size() >= 3);
        Key = UTF8ToWide(fields[0]);
        Lang = LanguageByNameStrict(fields[1]);
        Value = FromString<ui64>(fields[2]);
        for (size_t i = 3; i < fields.size(); ++i) {
            if (fields[i] == "form") {
                IsForm = true;
            }
            if (fields[i] == "titlecase") {
                Case = NPure::TitleCase;
            }
        }
    }

public:
    TPureLine(const TString& in, const TConfig& config) {
        switch (config.Format) {
            case TConfig::EFormat::OLD_READ:
                ParseFromOldRead(in, config);
                break;
            case TConfig::EFormat::OLD_WRITE:
                ParseFromOldWrite(in);
                break;
            case TConfig::EFormat::NEW:
                ParseFromNew(in);
                break;
            default:
                ythrow yexception() << "Not implemented";
        }
    }

    explicit TPureLine(const TPureIterator& iter)
        : Lang(LANG_MAX)
    {
        iter.Get(Key, IsForm, Case, Lang, Value);
    }

    TString ToString(const TConfig::EFormat& format) {
        switch (format) {
            case TConfig::EFormat::OLD_READ:
                {
                    int flag = (IsForm << 1) | (Case == NPure::TitleCase);
                    return TStringBuilder() << Key << "\t" << NameByLanguage(Lang) << "\t" << flag << "\t" << Value;
                }
            case TConfig::EFormat::OLD_WRITE:
                {
                    TStringBuilder result;
                    if (Lang != LANG_UNK) {
                        result << 'L' << IsoNameByLanguage(Lang) << '|';
                    }
                    result << (IsForm ? "!" : "") << Key << (Case == NPure::TitleCase ? "\x01" : "") << "\t" << Value;
                    return result;
                }
            case TConfig::EFormat::NEW:
                {
                    TStringBuilder result;
                    result << Key << '\t' << NameByLanguage(Lang) << '\t' << Value;
                    if (IsForm) {
                        result << "\tform";
                    }
                    if (Case == NPure::TitleCase) {
                        result << "\ttitlecase";
                    }
                    return result;
                }
            default:
                ythrow yexception() << "Not implemented";
        }
    }
};

//-------------------------------------------------------------------------------------
class TPureIgnoringFingerprints : public TPure {
private:
    class TImpl;
public:
    explicit TPureIgnoringFingerprints(const TString& trieFile) {
        TPure::Init(trieFile.data());
    }

    void Init(const TBlob& blob, const TPureOptions&) override;
};

class TPureIgnoringFingerprints::TImpl : public TPure::TImpl {
public:
    TImpl() {}

    TImpl(const TBlob& blob) {
        try {
            using namespace NPure::NImpl;

            THeader header(blob, DVM_DISABLED);
            Init(blob, header);
        } catch (NPure::TPureHeaderReader::TNoHeader& /*e*/) {
            PureImpl_ = NPure::GetPureImpl(blob, NPure::pureCompact);
            LemmatizedLanguages_.Reset();
        }
    }
};

void TPureIgnoringFingerprints::Init(const TBlob& blob, const TPureOptions&) {
    Impl.Reset(new TImpl(blob));
    TPureBase::Init(Impl->PureImpl());
}

//-------------------------------------------------------------------------------------

class TWriteablePureRecord : public NPure::NImpl::TGroupedByWordRecord {
public:
    void SetByLex(ui64 freq, int titleCase, ELanguage language = LANG_UNK)
        { SetFreq(false, freq, titleCase, language); }

    void SetByForm(ui64 freq, int titleCase, ELanguage language = LANG_UNK)
        { SetFreq(true, freq, titleCase, language); }

    typedef TImpl::iterator iterator;
    iterator Begin() { return Impl.begin(); }
    iterator End() { return Impl.end(); }
    void Sort() { ::Sort(Begin(), End()); }
    void Clear() { Impl.clear(); }

private:
    void SetFreq(bool byForm, ui64 freq, int titleCase, ELanguage language);
};

void TWriteablePureRecord::SetFreq(bool byForm, ui64 freq, int titleCase, ELanguage language) {
    if (freq >= (1ULL << 31)) {
        Cerr << "WARNING! Freqs are i32 values. " << freq << " exceeds 2^31 - 1.\n";
    }
    iterator pos = Begin();
    iterator const end = End();
    while (pos != end && pos->Lang() != language) {
        ++pos;
    }
    if (pos == end) {
        Impl.resize(Impl.size() + 1);
        pos = Impl.begin() + Impl.size() - 1;
    }
    pos->Lang() = language;
    pos->Freq(byForm, titleCase == NPure::TitleCase) = freq;
}

} //namespace

//---------------------------------------------------------------------------------------------------------------

namespace {
    typedef TCompactTrieBuilder<TChar, NPure::NImpl::TGroupedByWordRecord, TPureRecordPacker> TBuilder;
}

static void FlushKey(const TUtf16String& lastKey, TWriteablePureRecord& record, TBuilder& builder) {
    if (!lastKey.empty()) {
        record.Sort();
        builder.Add(lastKey, record);
    }
}

static std::pair<TBlob, TLangMask> ProcessFile(IInputStream& in, const TConfig& config) {
    THolder<TBuilder> builder(new TBuilder(
        CTBF_PREFIX_GROUPED |
        CTBF_UNIQUE |
        (config.Verbose ? CTBF_VERBOSE : CTBF_NONE)));

    TString line;
    TLangMask langs;
    TWriteablePureRecord record;
    TUtf16String lastKey;
    THolder<TTimer> buildTimer;
    if (config.Verbose) {
        buildTimer.Reset(new TTimer(TStringBuf("Building the trie took ")));
    }
    while (in.ReadLine(line)) {
        if (!line.empty())
            try {
                TPureLine pureLine(line, config);

                const ELanguage lang = pureLine.Lang;
                langs.SafeSet(lang);

                if (pureLine.Key != lastKey) {
                    FlushKey(lastKey, record, *builder);
                    lastKey = pureLine.Key;
                    record.Clear();
                }
                if (pureLine.IsForm) {
                    record.SetByForm(pureLine.Value, pureLine.Case, lang);
                } else {
                    record.SetByLex(pureLine.Value, pureLine.Case, lang);
                }
            } catch (const TPureLine::TBadLine& e) {
                Cerr << e.what() << Endl;
            }
    }
    FlushKey(lastKey, record, *builder);

    TBufferOutput output;
    builder->Save(output);
    if (config.Verbose) {
        buildTimer.Destroy();
    }
    builder.Destroy();
    if (!config.UseFastLayout) {
        return std::pair<TBlob, TLangMask>(TBlob::FromBuffer(output.Buffer()), langs);
    }
    TBufferOutput vEB;
    THolder<TTimer> timer;
    if (config.Verbose) {
        timer.Reset(new TTimer(TStringBuf("Making van Emde Boas layout took ")));
    }
    CompactTrieMinimizeAndMakeFastLayout<TPureRecordPacker>(vEB, output.Buffer().Data(), output.Buffer().Size(), config.Verbose);
    if (config.Verbose) {
        timer.Destroy();
        Cerr << "Size before van Emde Boas: " << output.Buffer().Size() << ", after: " << vEB.Buffer().Size() << Endl;
    }
    return std::pair<TBlob, TLangMask>(TBlob::FromBuffer(vEB.Buffer()), langs);
}

template <class TMapType>
static int checkLangMaps(const TMapType& mp1, const TMapType& mp2) {
    int retcode = 0;
    for (typename TMapType::const_iterator i = mp1.begin(); i != mp1.end(); ++i) {
        typename TMapType::const_iterator j = mp2.find(i->first);
        if (j == mp2.end()) {
            Cerr << "Trie header check failed on language " << NameByLanguage(i->first) << " no fingerprint present" << Endl;
            retcode = 1;
        } else if (i->second != j->second) {
            Cerr << "Trie header check failed on language " << NameByLanguage(i->first) << " fingerprints mismatch:"
            << "\n\t" << i->second << " " << j->second << Endl;
            retcode = 1;
        }
    }
    return retcode;
}

static int VerifyHeader(const TString& fingerprintsFile, const TString& triefile, const TLangMask& filter) {
    if (fingerprintsFile.empty()) {
        return 0;
    }
    TBlob trieBlob = TBlob::FromFileContent(triefile.data());
    NPure::TPureHeaderReader trBin(trieBlob);
    NPure::TPureHeaderWriter trText(fingerprintsFile.data());
    trText.FilterLanguages(filter);
    return checkLangMaps(trText.GetLangMap(), trBin.GetLangMap());
}

static int VerifyFile(const TString& inputFile, const TString& fingerprintsFile, const TString& triefile, const TConfig& config, const TLangMask& filter) {
    int retcode = VerifyHeader(fingerprintsFile, triefile, filter);
    TPureIgnoringFingerprints pure(triefile);

    TFileInput in(inputFile);
    size_t entrycount = 0;

    TString line;
    while (in.ReadLine(line)) {
        if (line.empty()) {
            continue;
        }
        try {
            TPureLine pureLine(line, config);
            entrycount++;
            ui64 trievalue = (pureLine.IsForm ?
                pure.GetByForm(pureLine.Key, pureLine.Case, pureLine.Lang)
                :
                pure.GetByLex(pureLine.Key, pureLine.Case, pureLine.Lang)
            ).GetFreq();
            if (trievalue != pureLine.Value) {
                Cerr << "Trie check failed on key #" << entrycount << " \"" << pureLine.Key << "\": value mismatch" << Endl;
                retcode = 1;
            }
        } catch (const TPureLine::TBadLine&) {
        }
    }

    for (TPureIterator iter = pure.Begin(); !iter.AtEnd(); iter.Step()) {
        --entrycount;
    }

    if (entrycount) {
        Cerr << "Broken iteration: entry count mismatch" << Endl;
        retcode = 1;
    }

    if (config.Verbose && !retcode) {
        Cerr << "Trie check successful" << Endl;
    }
    return retcode;
}

static void DumpHeader(const TString& triefile) {
    try {
        TBlob trieBlob = TBlob::FromFileContent(triefile.data());
        NPure::TPureHeaderReader trBin(trieBlob);
        for (NPure::TPureHeader::TLangMap::const_iterator i = trBin.GetLangMap().begin(); i != trBin.GetLangMap().end(); ++i)
            Cout << "Language: " << NameByLanguage(i->first) << ": " << i->second << "\n";
    } catch (const NPure::TPureHeaderReader::TNoHeader& ) {
    }
}

static void Dump(const TString& triefile, const TConfig& config) {
    if (config.DumpHeader)
        DumpHeader(triefile);
    TPureIgnoringFingerprints pure(triefile);
    if (config.DumpProtoHeader) {
        if(!pure.GetHeader())
            Cerr << "Warning: No proto header found!" << Endl;
        else
            Cout << pure.GetHeader()->Utf8DebugString() << Endl;
    }
    for(TPureIterator iter = pure.Begin(); !iter.AtEnd(); iter.Step()) {
        TPureLine pureLine(iter);
        if (config.DumpLanguage != LANG_UNK && pureLine.Lang != config.DumpLanguage)
            continue;
        if (config.DumpOnlyForms && !pureLine.IsForm)
            continue;
        Cout << pureLine.ToString(config.Format) << Endl;
    }
}

namespace {

struct TEncodingParser {
    ECharset operator() (const TString& name) const {
        ECharset result = CharsetByName(name.data());
        if (result == CODES_UNKNOWN) {
            throw yexception() << "Unrecognized encoding: \"" << name << "\"";
        }
        return result;
    }
};

struct TLanguageParser {
    ELanguage operator() (const TString& lang) const {
        if (lang.empty())
            return LANG_UNK;
        ELanguage result = LanguageByName(lang);
        if (result == LANG_UNK) {
            throw yexception() << "Unrecognized language: \"" << lang << "\"";
        }
        return result;
    }
};

} // Anonymous namespace.

int main(int argc, const char* argv[])
try {
    TConfig config;
    TString inputFile;
    TString fingerprintsFile;
    bool check = false;
    bool dump = false;

    NLastGetopt::TOpts options =  NLastGetopt::TOpts::Default();
    options.AddHelpOption();
    options.AddLongOption('b', "van-emde-boas", "Minimize and use van Emde Boas layout - the trie will be both minimized and became somewhat faster, "
        "because it is laid out in memory in a way that minimizes cache misses when going from root to leaf. "
        "But this makes many offsets bigger, so the trie becomes larger.").NoArgument().SetFlag(&config.UseFastLayout);
    options.AddLongOption('v', "verbose", "Be verbose - show progress & stats").NoArgument().SetFlag(&config.Verbose);
    options.AddLongOption('e', "encoding", "Define input stream encoding").RequiredArgument("<enc>").DefaultValue("utf-8").
        StoreMappedResultT<TString>(&config.Encoding, TEncodingParser());
    options.AddLongOption('i', "input", "Read input from file (use stdin if not specified)").RequiredArgument("<file>").
        StoreResult(&inputFile);
    options.AddLongOption('f', "fingerprint", "Read morphology fingerprints from file").RequiredArgument("<file>").
        StoreResult(&fingerprintsFile);
    options.AddLongOption('c', "check", "Check the resulting trie (requires -i)").NoArgument().SetFlag(&check);
    options.AddLongOption('l', "language", "Compile trie with language").NoArgument().SetFlag(&config.Languages);
    options.AddLongOption('d', "dump", "Dump trie content").NoArgument().SetFlag(&dump);
    options.AddLongOption('p', "dump_proto_header", "dump v2 proto header if exists").NoArgument().SetFlag(&config.DumpProtoHeader);
    options.AddLongOption("dump-forms", "dump only forms, not lemmas").NoArgument().SetFlag(&config.DumpOnlyForms);
    options.AddLongOption("dump-header", "dump header").NoArgument().SetFlag(&config.DumpHeader);
    options.AddLongOption("dump-language", "dump data only for this language").RequiredArgument("<lang>").DefaultValue("").
        StoreMappedResultT<TString>(&config.DumpLanguage, TLanguageParser());
    options.AddLongOption("format", "format for reading/writing (OLD_READ, OLD_WRITE, NEW)").DefaultValue("");
    options.SetFreeArgsMin(1);
    options.SetFreeArgsMax(1);
    options.SetFreeArgTitle(0, "<trie file>", "The filename for the compiled pure.");

    NLastGetopt::TOptsParseResultException res(&options, argc, argv);
    TString triefile = res.GetFreeArgs()[0];

    TString format = res.Get("format");
    if (format == "OLD_READ") {
        config.Format = TConfig::EFormat::OLD_READ;
    } else if (format == "OLD_WRITE") {
        config.Format = TConfig::EFormat::OLD_WRITE;
    } else if (format == "NEW") {
        config.Format = TConfig::EFormat::NEW;
    } else if (dump) {
        config.Format = TConfig::EFormat::OLD_WRITE;
    } else {
        config.Format = TConfig::EFormat::OLD_READ;
    }

    if (dump) {
        Dump(triefile, config);
        return 0;
    }

    THolder<NPure::TPureHeaderWriter> header;

    if (fingerprintsFile.empty()) {
        header.Reset(new NPure::TPureHeaderWriter);
    } else {
        header.Reset(new NPure::TPureHeaderWriter(fingerprintsFile.data()));
    }

    THolder<TFileInput> fileIn;
    TBufferedInput cIn(&Cin);

    IInputStream* in = &cIn;
    if (!inputFile.empty()) {
        fileIn.Reset(new TFileInput(inputFile));
        in = fileIn.Get();
    }

    std::pair<TBlob, TLangMask> pureInfo = ProcessFile(*in, config);
    header->FilterLanguages(pureInfo.second);

    {
        TFixedBufferFileOutput outTrie(triefile);
        header->WriteHeader(outTrie);
        outTrie.Write(pureInfo.first.Data(), pureInfo.first.Size());
    }

    if (check)
        return VerifyFile(inputFile, fingerprintsFile, triefile, config, pureInfo.second);

    return 0;
} catch (const NLastGetopt::TException& e) {
    Cerr << e.what() << Endl;
    return 1;
} catch (const yexception& e) {
    Cerr << e.what() << Endl;
    return 2;
} catch (const std::exception& e) {
    Cerr << e.what() << Endl;
    return 3;
} catch (...) {
    Cerr << "Unknown exception!" << Endl;
    return 4;
}
