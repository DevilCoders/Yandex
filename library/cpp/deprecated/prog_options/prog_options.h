#pragma once

#include <library/cpp/deprecated/split/delim_string_iter.h>
#include <library/cpp/containers/dictionary/dictionary.h>

#include <util/folder/dirut.h>
#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/generic/yexception.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>

#include <utility>

/*
  optSpec specifies options in the following format:
    "|<opt-name1>|+|<opt-name2>|<opt-name3>|?|<opt-name4>|*|..."
    <opt-name> is Case-sensitive!
    if + is present after <opt-name> that means an option value must be provided
    if # is present after <opt-name> that means an option value must be provided and there can be several such options
    if ? is present after <opt-name> that means an option value may be provided
    if * is present after <opt-name> that means an option value may be provided
        and this option value can consist of several words
        (present limitation: this option can only be the last when passed to program)
*/

/* (not implemented)
    Command-line options are specified as:
        -opt_name1 <opt_value1> -opt_name2 -opt_name3 <opt_value3>

    Options in file are specified as:
        opt_name1 \t <opt_value1>
        opt_name2
        opt_name3 \t <opt_value3>
*/

class TProgOptionsException: public yexception {
public:
    TProgOptionsException(const char* what) {
        *this << what;
    }
};

using TOptRes = std::pair<bool, const char*>;
using TDirOptRes = std::pair<bool, TString>;

class TProgramOptions {
public:
    using TOptions = THashMap<TString, const char*>;

    struct TOptArg {
        TString Name;
        bool Check;

        TOptArg(const char* name, bool check = true)
            : Name(name)
            , Check(check)
        {
        }

        TOptArg(const TString& name, bool check = true)
            : Name(name)
            , Check(check)
        {
        }
    };

public:
    TProgramOptions(const TString& optSpec)
        : OptSpec(optSpec)
    {
    }

    // Init from command-line
    void Init(int argc, const char* argv[]);

    // returns pair (present,arg_value) arg_value is 0 if option has no argument
    TOptRes GetOption(const TOptArg& optArg) const {
        TOptionsConstIter it = GetOptionsIter(optArg);

        return (it == Options.end()) ? std::make_pair(false, (const char*)nullptr) : std::make_pair(true, it->second);
    }

    bool HasOption(const TOptArg& optArg) const {
        return GetOptionsIter(optArg) != Options.end();
    }

    bool HasMultOption(const TOptArg& optArg) const {
        return GetMultOptionsIter(optArg) != MultOptions.end();
    }

    const char* GetReqOptVal(const TString& optName) const {
        TOptionsConstIter it = GetOptionsIter(TOptArg(optName, true));
        if (it == Options.end())
            throw TProgOptionsException(("Required option '" + optName + "' not specified").data());
        return it->second;
    }

    template <class T>
    T GetReqOptVal(const TString& optName) const {
        return FromString<T>(GetReqOptVal(optName));
    }

    // returns specified value if option was specified or defVal if not
    const char* GetOptVal(const TOptArg& optArg, const char* defVal) const {
        TOptionsConstIter it = GetOptionsIter(optArg);
        return (it == Options.end()) ? defVal : it->second;
    }

    // returns specified value if option was specified or defVal if not
    template <class T>
    T GetOptVal(const TOptArg& optArg, const T& defVal) const {
        TOptionsConstIter it = GetOptionsIter(optArg);
        return (it == Options.end()) ? defVal : FromString<T>(it->second);
    }

    // returns pair (present,arg_value) arg_value is 0 if option has no argument
    TDirOptRes GetDirOption(const TOptArg& optArg) const {
        TOptRes res = GetOption(optArg);
        if (!res.first)
            return std::make_pair(false, "");

        TString dirName = res.second;
        //if (!correctpath(dirName))
        //    throw yexception("Specified dirname %s for option %s is invalid", dirName.c_str(), optArg.Name.c_str());
        SlashFolderLocal(dirName);

        return std::make_pair(true, dirName);
    }

    // returns specified value if dir option was specified or defVal if not
    TString GetDirOptVal(const TOptArg& optArg, const TString& defVal) const {
        TDirOptRes res = GetDirOption(optArg);
        return (res.first) ? res.second : defVal;
    }

    TString GetReqDirOptVal(const TString& optName) const {
        TDirOptRes res = GetDirOption(TOptArg(optName, true));
        if (!res.first)
            ythrow yexception() << "Required option '" << optName.data() << "' not specified";
        return res.second;
    }

    const TOptions& GetAllNamedOptions() const {
        return Options;
    }

    const TVector<const char*>& GetAllUnnamedOptions() const {
        return UnnamedOptions;
    }

    const TVector<const char*>& GetMultOptions(const TOptArg& optArg) const {
        TMultOptions::const_iterator it = GetMultOptionsIter(optArg);
        return (it == MultOptions.end()) ? MultOptionsDummy : it->second;
    }

private:
    static const char* OPT_SPEC_VALUES;

    TString OptSpec;

    using TOptionsConstIter = TOptions::const_iterator;

    using TMultOptions = THashMap<TString, TVector<const char*>>;

    TOptions Options;
    TVector<const char*> UnnamedOptions;
    TMultOptions MultOptions;
    TVector<const char*> MultOptionsDummy; // just some empty vector

    TString LongOption;

private:
    // returns *?+ (see above for meaning) or ' ' - option w/o params, or '\0' - no such option (if not optArg.Check)
    char GetOptionSpec(const TOptArg& optArg) const {
        size_t optPos = OptSpec.find(TString("|") + optArg.Name + "|");
        if (optPos == TString::npos) {
            if (optArg.Check)
                throw TProgOptionsException(("Incorrect option (not in spec): " + optArg.Name).data());
            else
                return '\0';
        }

        size_t markPos = optPos + 1 + optArg.Name.length() + 1;
        if ((markPos == OptSpec.size()) || (strchr(OPT_SPEC_VALUES, OptSpec[markPos]) == nullptr)) {
            return ' ';
        }
        return OptSpec[markPos];
    }

#define GET_OPTIONS_ITER(CLASSTYPE)                                                  \
    T##CLASSTYPE::const_iterator Get##CLASSTYPE##Iter(const TOptArg& optArg) const { \
        if (optArg.Check)                                                            \
            Y_UNUSED(GetOptionSpec(optArg));                                         \
        return CLASSTYPE.find(optArg.Name);                                          \
    }

    GET_OPTIONS_ITER(Options);
    GET_OPTIONS_ITER(MultOptions);

#undef GET_OPTIONS_ITER
};

inline int main_with_options_and_catch(TProgramOptions& progOptions, int argc, const char* argv[],
                                       int (*actual_main)(TProgramOptions&),
                                       void (*print_help)() = nullptr) {
    try {
        progOptions.Init(argc, argv);

        if (progOptions.HasOption(TProgramOptions::TOptArg("h", false)) ||
            progOptions.HasOption(TProgramOptions::TOptArg("help", false))) {
            if (print_help) {
                print_help();
            }
            return 0;
        }

        return actual_main(progOptions);

    } catch (TProgOptionsException& e) {
        Cerr << "Error while parsing program options: " << e.what() << Endl;
        if (print_help) {
            print_help();
        }
    } catch (std::exception& e) {
        Cerr << "Standard exception has occurred: " << e.what() << Endl;
    } catch (...) {
        Cerr << "ERROR: Unknown exception has occurred!\n";
    }
    return -1;
}

// all set by default
template <class E, size_t N, class TBitField>
inline void SetEnumFlags(const std::pair<const char*, E> (&str2Enum)[N], TOptRes optSpec,
                         TBitField& res, bool allIfEmpty = true) {
    TStringBuf optVal;
    if (optSpec.first)
        optVal = optSpec.second;
    SetEnumFlags(str2Enum, optVal, res, allIfEmpty);
    //SetEnumFlags(str2Enum, optSpec.first ? TStringBuf(optSpec.second) : TStringBuf(), res, allIfEmpty);
}

// all set by default
template <class E, class TBitField>
inline void SetEnumFlags(const TStringIdDictionary<E>& dict, TOptRes optSpec,
                         TBitField& res, bool allIfEmpty = true) {
    TStringBuf optVal;
    if (optSpec.first)
        optVal = optSpec.second;
    SetEnumFlags(dict, optVal, res, allIfEmpty);
    //SetEnumFlags(str2Enum, optSpec.first ? TStringBuf(optSpec.second) : TStringBuf(), res, allIfEmpty);
}

template <class T>
inline void SetParts(const T& parts, const TOptRes& opt, bool blackList, /* out */ THashSet<TString>& setParts) {
    Y_ASSERT(opt.first || !blackList);
    THashSet<TString> availParts;

    for (const auto& p : parts) {
        if (opt.first && !blackList) {
            availParts.insert(p);
        } else {
            setParts.insert(p);
        }
    }

    if (opt.first) {
        for (TDelimStringIter it(TStringBuf(opt.second), ","); it.Valid(); ++it) {
            TString part = ToString(*it);
            if (!blackList && !availParts.contains(part))
                ythrow yexception() << "incorrect part: " << part;
            if (blackList) {
                setParts.erase(part);
            } else {
                setParts.insert(part);
            }
        }
    }
}

// all set by default
template <class T>
inline void SetParts(const T& parts, const TOptRes& opt, /* out */ THashSet<TString>& setParts) {
    SetParts<T>(parts, opt, false, setParts);
}
