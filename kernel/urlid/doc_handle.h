#pragma once

#include "doc_route.h"

#include <kernel/index_generation/constants.h>

#include <util/generic/fwd.h>
#include <util/generic/variant.h>
#include <util/string/cast.h>

class IInputStream;
class IOutputStream;

/*! @brief A class storing a document ID and providing functions to de/serialize it.
 *
 * A DocHandle uniquely identifies a document in the current context.
 * For base searches, a DocHandle contains only hash.
 * For metasearches, a DocHandle contains also a route to the base search.
 * This class defines how these entities are encoded into a string so that all search components use the same string format
 * and the same code for encoding/decoding it.
 *
 * To serialize an object @c handle of this class, you can use:
 * * handle.ToString(...)
 * * IOutputStream str; str << handle; // string, like ToString(PrintAll)
 * * IOutputStream str; ::Save (str, handle); // fixed binary format
 *
 * To deserialize an object, use functions that are symmetrical to the serializers.
 *
 * Binary-encoded docids may be written as docroute/docid, i.e. -17574972571570/2018915346.
 * Use /tools/doc_handle_decode to decode this to a human-readable form, i.e. 1-2-3-4-Z0000000012345678
 *
 * @see https://st.yandex-team.ru/SEARCH-2286
 * @see https://nda.ya.ru/3SBdvW — инструкция, как перейти от строк к этому классу.
 */
class TDocHandle {
public:
    using THash = ui64;
    using TIndexGen = ui32;
    static const THash InvalidHash;

    ui32 IndexGeneration = UndefIndGenValue;
    std::variant<THash, TString> DocHash = InvalidHash;
    TDocRoute DocRoute;

    //! Format options for ToString
    /*! @note You can explicitly pass these flags to @c ToString, the defaults are below (@see ToString)
     *        There is no way to pass these flags to @c Out<TDocHandle>, the default there is PrintAll
     *
     * The hash is always printed, whatever the flag value is.
     * To control the format of the hash, use @c SetUnique and @c SetIsString
     * All supported combinations of the values are listed in the enum,
     * other combinations may behave unexpectedly (and may work — test/RTSL before usage).
     */
    enum EFormat {
        //! if (IndexGeneration != UndefIndGenValue), append ("," + ToString(IndexGeneration)) to the resulting string
        AppendIndGeneration = 1 << 0,
        //! Print route if it is not empty
        PrintRoute = 1 << 1,
        //! Print universally unique id for this handle.
        //! If IsUnique(), then print DocHash only, otherwise works exactly as PrintRoute.
        //! This format is used for generating URL queries.
        PrintZDocId = 1 << 2,
        //! Do not print 'Z' in front of hash, even if IsUnique().
        OmitZ = 1 << 3,
        //! Print the full docid with route, omitting the first node from the route.
        PrintClientDocId = 1 << 4 | PrintRoute,

        PrintAll = PrintRoute | AppendIndGeneration,
        PrintHashOnly = 0, //< but respect format flags stored in DocRoute
        PrintZDocIdOmitZ = PrintZDocId | OmitZ,
        PrintCliendDocIdWithIndex = PrintClientDocId | AppendIndGeneration
    };

    TDocHandle(const TDocHandle&) = default;
    //! Create this object from a CGI-parameter-like string
    explicit TDocHandle(const TStringBuf& str) { FromString(str); }
    explicit TDocHandle(const TString& str) { FromString(str); }
    explicit TDocHandle(const char* str) { FromString(str); }
    //! Copy another TDocHandle, ignoring route and index generation
    explicit TDocHandle(std::variant<THash, TString> parsedDocHash) : DocHash(parsedDocHash) {}
    explicit TDocHandle(THash docHash = InvalidHash, TDocRoute docRoute = TDocRoute(), TIndexGen indGen = UndefIndGenValue)
    : IndexGeneration (indGen), DocHash (docHash), DocRoute (docRoute) {
    }
    //! Create Z-docid (i.e. UUID) with the given parameters
    static TDocHandle ZDocId(THash docHash = InvalidHash, TDocRoute docRoute = TDocRoute(), TIndexGen indGen = UndefIndGenValue) {
        TDocHandle ret(docHash, docRoute, indGen);
        ret.SetUnique();
        return ret;
    }
    inline bool operator==(const TDocHandle& rhs) const {
        return DocHash == rhs.DocHash && DocRoute == rhs.DocRoute;
    }
    inline bool operator!=(const TDocHandle& rhs) const {
        return !(*this == rhs);
    }
    bool operator!() const noexcept {
        return DocHash == std::variant<THash, TString>(InvalidHash) && !DocRoute;
    }
    explicit operator bool() const noexcept {
        return !!*this;
    }
    TDocHandle& operator=(const TDocHandle& rhs) {
        DocHash = rhs.DocHash;
        DocRoute.Raw() = rhs.DocRoute.Raw();
        IndexGeneration = rhs.IndexGeneration;
        return *this;
    }
    //! This docid as a CGI parameter
    /*! @note if DocRoute.IsUnique(), this function prepends 'Z' to the hash.
     * Z-docids are serialized like "Z00000000831A0000",
     * non-Z docid with the same value will be serialized as "6787"
     */
    TString ToString(EFormat format = PrintRoute) const;
    //! Parse a string (like a CGI parameter) and fill this object with values from it
    /*! @note This function can handle hashes both with and without the letter 'Z' */
    void FromString(TStringBuf strDocHandle);
    //! Return numeric representation of the hash. This function may throw, if the hash is not convertible to a number.
    /*! Hashes are converted using the following rules:
     * 1. Z-docids are treated as hex-numbers with Z prepended. They must consist of exactly 16 hex characters (case-insensitive)
     *    They are converted to int values using NUrlId::Str2Hash.
     * 2. Non-format Z-docids and non-Z docids are treated as decimal numbers, no hex digits are allowed.
     *    They are converted to int values using ::FromString.
     *
     * Usually, if this->IsString(), this function will throw, because all the parsing has been done in FromString.
     * However, for non-conforming Z-docids containing N hex digits, where (N<16 and N%2 == 0), this function will yield a number even if IsString().
     * If this is not a Z-docid, but it had leading zeros when parsing, this function will also yield int, though IsString() is true.
     */
    THash IntHash() const;
    //! Just like @c IntHash, but without using exceptions
    bool TryIntHash(THash& result) const;
    //! Whether it's a z-docid
    inline bool IsUnique() const noexcept {
        return !DocRoute.GetFlag(0);
    }
    //! Set whether it's a z-docid
    inline void SetUnique(bool b = true) noexcept {
        DocRoute.SetFlag(0, !b);
    }
    //! Use StrDocHash instead of DocHash
    /*! This fallback mode is used for hashes that will be serialized incorrectly if parsed to an int:
     * - Z-docids consisting of N hex-digits, where N != 16
     * - Non-z-docids that have leading zeros
     * - All other docids with unrecognized formats (non-z-docids with hex numbers, Z-docids that are N hex digits where N > 16 or N % 2 != 0).
     */
    inline bool IsString() const noexcept {
        return std::holds_alternative<TString>(DocHash);
    }

    TDocRoute::TSourceType ClientNum() const {
        return DocRoute.Next();
    }

    //! Return the index of the base search that found the document, i.e. the first hop in the route
    inline TDocRoute::TSourceType BaseSearchNum() const {
        return DocRoute.Source(0);
    }

    /// Return a copy of this docid without the topmost client number
    TDocHandle ClientDocId() const {
        TDocHandle ans(*this);
        ans.DocRoute.PopFront();
        return ans;
    }

    /// Add a new router to the handle, like this: client-docid
    void PrependClientNum(TDocRoute::TSourceType client) {
        DocRoute.PushFront(client);
    }

    void Save(IOutputStream* str) const;
    void Load(IInputStream* str);
};

inline
TString ToString(const TDocHandle& hndl, const TDocHandle::EFormat fmt = TDocHandle::PrintRoute) {
    return hndl.ToString(fmt);
}
