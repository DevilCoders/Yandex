#pragma once

#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NBlackbox2 {
    /// Default argument value in function signatures
    extern const TString EMPTY_STR;

    /// Single blackbox request option
    class TOption {
    public:
        TOption(const TString& key, const TString& val)
            : Key_(key)
            , Val_(val)
        {
        }

        const TString& Key() const {
            return Key_;
        }
        const TString& Val() const {
            return Val_;
        }

    private:
        TString Key_, Val_;
    };

    // Standard blackbox options

    extern const TOption OPT_REGNAME;       // show name as used for registration
    extern const TOption OPT_AUTH_ID;       // return authid information
    extern const TOption OPT_FULL_INFO;     // return full user info in login and sessionid

    extern const TOption OPT_VERSION2;     // use login version 2 mode
    extern const TOption OPT_MULTISESSION; // use multisession response format

    extern const TOption OPT_GET_SOCIAL_ALIASES; // get social aliases info
    extern const TOption OPT_GET_ALL_ALIASES;    // get all aliases info

    extern const TOption OPT_GET_ALL_EMAILS;    // get all known user emails
    extern const TOption OPT_GET_YANDEX_EMAILS; // get all user Yandex emails
    extern const TOption OPT_GET_DEFAULT_EMAIL; // get default email

    extern const TOption OPT_GET_USER_TICKET; // get user ticket

    /// Blackbox request options list
    class TOptions {
    public:
        TOptions() = default;
        TOptions(const TOption& opt);

        typedef TVector<TOption> ListType;

        /// Return options list
        const ListType& List() const {
            return Options_;
        }

        /// Add option to the list
        TOptions& operator<<(const TOption& opt);

    private:
        ListType Options_;
    };

    /// Default value for function signatures, no options
    extern const TOptions OPT_NONE;

    // Forward declaration of BlackBox response class
    class TResponse;

    /// DB_fields options, inout parameter of blackbox methods
    class TDBFields {
    public:
        typedef TMap<TString, TString> TContainerType;

        TDBFields(const TString& field = EMPTY_STR); // add one field at construction for convenience
        TDBFields(const TResponse* resp);            // init fields and values by response

        void ReadResponse(const TResponse* resp); // parse the response and get values

        bool Empty() const {
            return Fields_.empty();
        } // true if no fields inside
        size_t Size() const {
            return Fields_.size();
        } // number of fields inside

        void Clear();       // clear all stored dbfields
        void ClearValues(); // clear stored values but keep field names

        TDBFields& operator<<(const TString& field); // add field

        const TString& Get(const TString& field) const; // get field value by name
        bool Has(const TString& field) const;           // has field

        TContainerType::const_iterator Begin() const; // iterate over fields
        TContainerType::const_iterator End() const;   // needed for *-fields

        TContainerType::const_iterator begin() const {
            return Begin();
        }

        TContainerType::const_iterator end() const {
            return End();
        }

        operator TOptions() const; // to pass DBFields as Options

    private:
        TContainerType Fields_;
    };

    TOptions& operator<<(TOptions& options, const TDBFields& fields);

    /// Test email request option
    class TOptTestEmail {
    public:
        TOptTestEmail(const TString& addr)
            : AddrToTest_(addr)
        {
        }
        TOptions& Format(TOptions& opts) const;

    private:
        TString AddrToTest_;
    };

    TOptions& operator<<(TOptions& options, const TOptTestEmail& opt);

    /// Aliases request option
    class TOptAliases {
    public:
        TOptAliases(const TString& alias = EMPTY_STR); // add one alias at construction for convenience

        bool Empty() const {
            return Aliases_.empty();
        } // true if no aliases inside
        size_t Size() const {
            return Aliases_.size();
        } // number of aliases inside

        TOptAliases& operator<<(const TString& alias); // add alias

        operator TOptions() const; // to pass aliases as Options
        TOption Format() const;    // format request option
    private:
        TSet<TString> Aliases_;
    };

    TOptions& operator<<(TOptions& options, const TOptAliases& opt);

    /// Attributes request option and accessor
    class TAttributes {
    public:
        typedef TMap<TString, TString> TContainerType;

        TAttributes(const TString& attr = EMPTY_STR); // add one attr at construction for convenience
        TAttributes(const TResponse* resp);           // init fields and values by response

        void ReadResponse(const TResponse* resp); // parse the response and get values

        bool Empty() const {
            return Attributes_.empty();
        } // true if no fields inside
        size_t Size() const {
            return Attributes_.size();
        } // number of fields inside

        void Clear();       // clear all stored attributes
        void ClearValues(); // clear stored values but keep field names

        TAttributes& operator<<(const TString& attr); // add attribute

        const TString& Get(const TStringBuf attr) const; // get attribute value by name
        bool Has(const TStringBuf attr) const;           // has attribute

        TContainerType::const_iterator Begin() const; // container iterators
        TContainerType::const_iterator End() const;   // container iterators

        operator TOptions() const; // to pass Attributes as Options
        TOption Format() const;    // format request option

    private:
        TContainerType Attributes_;
    };

    TOptions& operator<<(TOptions& options, const TAttributes& attrs);
}
