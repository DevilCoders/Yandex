#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/system/types.h>
#include <util/system/yassert.h>

#include <initializer_list>
#include <type_traits>

//! Information about the route of a document from a base search to a meta search
/*! The doc route is a sequence of sources starting from the highest metasearch.
 * route.Next() or route.Source (route.Length() - 1) corresponds to the next int search to transmit the request to,
 * route.Source (0) corresponds to the basesearch id.
 *
 * The route is currently stored in a single ui64 bitmask, the storage format is an implementation detail
 * and might change in future (it'll require change in protobuf also):
 *  bits  | meaning
 *      0 | 0 for a route of Z-docid, 1 [default] for a route of a local docid.
 *      1 | 0 if TDocHandle should use string format instead of UrlHash
 *  2.. 3 | unused flags
 *  4..13 | id of the basesearch
 * 14..23 | id of the 1st int search
 * ... and so on, 10 bits per level ...
 * 54..63 | id of the highest meta search
 *
 * To convert TDocRoute -> DocId, use TDocRoute::ToDocId
 * To convert DocId -> TDocRoute, use TDocRoute::FromDocId
 * @see https://st.yandex-team.ru/SEARCH-2286
 */
class TDocRoute {
public:
    //! The type of underlying binary representation. May change in future (although it's unlikely)
    //! This type may become signed or unsigned, do not rely on it.
    using TRawType = i64;
    using TRawTypeUnsigned = std::make_unsigned<TRawType>::type;
    //! The minimal type that can hold any acceptable source number.
    using TSourceType = ui16;

    static constexpr ui16 MaxLength = 6; //!< Maximal supported hierarchy levels
    static constexpr ui8 TotalFlags = 4; //!< How many flags the route can hold
    //! Maximal value for source that fits capacity. (NoSource <= MaxSourceNumber) is true.
    static constexpr TRawType MaxSourceNumber = 0x3FF;
    //! The value that indicates an unfilled source value. NoSource is <= than MaxSourceNumber.
    static constexpr TSourceType NoSource = static_cast<TSourceType>(-1) & MaxSourceNumber;
    static constexpr char Separator = '-'; //!< Separates hierarchy levels and sources from DocHash in a DocId.

    //! By default all flags are set to 1 and no route information is stored
    TDocRoute(TRawType value = static_cast<TRawType>(-1)) noexcept : Value_(value) {}
    TDocRoute(const TDocRoute&) = default;
    TDocRoute& operator=(const TDocRoute&) = default;
    //! Initialize the route with the list of sources. The first element is the base search, then come metasearches.
    TDocRoute(std::initializer_list<TSourceType> lst) noexcept {
        Y_ASSERT(lst.size() <= MaxLength);
        for (ui16 src : lst) {
            PushFront(src);
        }
    }

    bool operator==(TDocRoute rhs) const noexcept {
        return Value_ == rhs.Value_; //< the flags are also compared
    }
    bool operator!=(TDocRoute rhs) const noexcept {
        return !(*this == rhs);
    }
    // test for an empty route, ignoring flags. Works faster than `Length() == 0`.
    bool operator!() const noexcept {
        constexpr TRawTypeUnsigned emptyMask = static_cast<TRawTypeUnsigned>(-1) & ~static_cast<TRawTypeUnsigned>(0xF); //< exclude flags from the mask
        return (static_cast<TRawTypeUnsigned>(Value_) & emptyMask) == emptyMask;
    }
    // test for an empty route, ignoring flags. Works faster than `Length() == 0`. Returns true for non-empty routes.
    explicit operator bool() const noexcept {
        return !!*this;
    }

    //! Parse the docid of the form 01-02-ABCDEF, where the sources are dash-prepended.
    /*! The leftmost source is the highest one in the hierarchy, the sources are decimal numbers.
     * If there are more than MaxLength sources, the yexception is thrown.
     * @c strDocId and docHash may be the same object.
     * @throws yexception in case of format error
     */
    static TDocRoute FromDocId(const TStringBuf& strDocId, TStringBuf* /*out*/ docHash = nullptr);
    //! Convert this value to the string DocId format, like: AAA-BB-CCC
    /*! Each source is encoded as a decimal number, then the sources are placed into a string,
     * where the leftmost number corresponds to the upper source in the hierarchy,
     * and the rightmost source is the basesearch number.
     * To get a DocId as a valid CGI parameter, you need to append Separator (like "-") and DocHash.
     * If Length() == 0, then an empty string is returned.
     */
    TString ToDocId() const;
    //! Raw binary data representation for protobuf. May change in future, do not rely on it.
    TRawType Raw() const noexcept {
        return Value_;
    }
    TRawType& Raw() noexcept {
        return Value_;
    }
    //! Get the number of the source at the given hierarchy @c level (0 is basesearch)
    /*! @note this function works fast and does not check @c level range.
     *  If the level is higher than MaxLength - 1, the result will likely be 0, in all cases it will be senseless;
     *  else if the level is higher than Length() - 1, the result will be -1 (no source)
     */
    inline TSourceType Source(ui8 level) const {
        Y_ASSERT(level < MaxLength);
        level *= BitsPerSource_;
        level += TotalFlags;
        return (static_cast<TRawTypeUnsigned>(Value_) >> level) & MaxSourceNumber;
    }
    //! Return the highest available source (the farthest from the basesearch), or NoSource if Length() == 0
    inline TSourceType Next() const {
        const ui8 l = Length();
        return l > 0? Source(l - 1) : NoSource;
    }
    //! Add a new uppermost router
    /*! @note You have to check that Length() < MaxLength beforehead or the function will silently fail,
     *        spoiling flags and without actually recording @c nextNode anywhere
     */
    inline void PushFront(TSourceType nextNode) {
        Y_ASSERT(nextNode <= MaxSourceNumber);
        ui8 length = Length();
        if (length < MaxLength) {
            SetSource(length, nextNode);
        }
    }
    //! Remove the uppermost router
    /*! @note Does nothing on an empty route
     */
    inline void PopFront() {
        const ui8 l = Length();
        if (l > 0) {
            SetSource(l - 1, NoSource);
        }
    }
    //! Expand incomplete chain ending with intsearch toward basesearch
    /*! @note You have to check that Length() < MaxLength beforehead or the function will silently fail
     *        without actually recording @c nextNode anywhere
     */
    inline void PushBack(TSourceType nextNode) {
        Y_ASSERT(nextNode <= MaxSourceNumber);
        if (Source(MaxLength - 1) == NoSource) {
            const auto flags = Value_ & AllFlagsMask_;
            Value_ = ((Value_ & ~AllFlagsMask_) << BitsPerSource_) | (nextNode << TotalFlags) | flags;
        }
    }
    //! Set the number of the source # @c level at the hierarchy (0 is basesearch).
    /*! @note this function does not check arguments for correctness.
     *        level should be < MaxLength;
     *        value should be <= MaxSourceNumber.
     * Otherwise you'll either spoil the flags or get your source number truncated.
     */
    inline void SetSource(ui8 level, TSourceType value) {
        Y_ASSERT(level < MaxLength);
        Y_ASSERT(value <= MaxSourceNumber);

        level *= BitsPerSource_;
        level += TotalFlags;
        value &= MaxSourceNumber;
        Value_ &= ~(MaxSourceNumber << level); //< clear the previous value
        Value_ |= (static_cast<TRawType>(value) << level);
    }
    //! Set source # @c level to NoSource. No range checking is performed.
    inline void ClearSource(ui8 level) {
        Y_ASSERT(level < MaxLength);
        return SetSource(level, NoSource);
    }
    //! Delete all sources, but preserve flags
    inline void Clear() {
        const auto flags = GetAllFlags();
        Value_ = static_cast<TRawType>(-1);
        SetAllFlags(flags);
    }
    //! Get the number of the levels currently filled.
    /*! @note If there are gaps, the returned value is the length of the continuous chain from Source(0),
     * the filled upper routers after gaps are not counted.
     */
    inline ui8 Length() const {
        ui8 ans = 0;
        while (ans < MaxLength && Source(ans) != NoSource)
            ++ans;
        return ans;
    }
    inline bool GetFlag(ui8 index) const {
        Y_ASSERT(index < TotalFlags);
        return Value_ & (static_cast<TRawType>(1) << index);
    }
    inline void SetFlag(ui8 index, bool value) {
        Y_ASSERT(index < TotalFlags);
        const TRawType mask = Value_ & (static_cast<TRawType>(1) << index);
        if (value) {
            Value_ |= mask;
        } else {
            Value_ &= ~mask;
        }
    }
    //! Get a canonical representation of all sources without any flags
    inline TDocRoute GetOnlySources() const {
        return TDocRoute(Value_ & ~AllFlagsMask_);
    }
private:
    TRawType Value_ = -1;
    static constexpr unsigned BitsPerSource_ = 10;
    static constexpr TRawTypeUnsigned AllFlagsMask_ = ~(static_cast<TRawTypeUnsigned>(-1) << TotalFlags);
    ui8 GetAllFlags() const noexcept {
        return Value_ & AllFlagsMask_;
    }
    void SetAllFlags(ui8 value) noexcept {
        Value_ &= ~AllFlagsMask_;
        Value_ |= value & AllFlagsMask_;
    }
};
