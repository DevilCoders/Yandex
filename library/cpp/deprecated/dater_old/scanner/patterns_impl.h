#pragma once

#include <cctype>
#include <cstring>
#include <cmath>
#include <numeric>

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>

#include "scanner.h"

namespace NDater {
    namespace NPrivate {
        template <typename Char>
        inline ui64 Atoi(const Char* begin, const Char* end) {
            ui64 res = 0;
            while (begin < end)
                res = res * 10 + ((*begin++) - '0');
            if (!res && end - begin > 2)
                return -1;
            return res;
        }

        struct TNum {
            ui64 Num;
            bool Word;

        public:
            explicit TNum(ui64 n = 0, bool word = false)
                : Num(n)
                , Word(word)
            {
            }

            operator ui64() {
                return Num;
            }
        };

        template <typename TItem>
        class TFixedStack {
            TItem Array[3];
            ui32 Set;

        public:
            TFixedStack()
                : Array()
                , Set()
            {
            }

            void Push(TItem i = 0) {
                if (Set > 2) {
                    Array[0] = Array[1];
                    Array[1] = Array[2];
                    Array[2] = i;
                } else {
                    Array[Set++] = i;
                }
            }

            TItem& operator[](int i) {
                Y_ASSERT(i >= 0 && i < 3);
                return Array[i];
            }
        };

        template <typename Char, bool IsUrl = false>
        struct TScanContext {
            const Char*& p;
            const Char*& pe;

            const Char* p0;
            const Char* begin;

            int cs;
            bool fail;
            TCoord sep;
            TFixedStack<TNum> nums;
            TFixedStack<TCoord> coords;

        public:
            TScanContext(const Char*& _p, const Char*& _pe)
                : p(_p)
                , pe(_pe)
                , p0(_p)
                , begin(_p)
                , cs()
                , fail()
                , sep()
                , nums()
                , coords()
            {
            }

            void SetBegin() {
                begin = p;
            }

            void PushNum() {
                PushNum(Atoi(begin, p), false);
            }

            void PushNum(ui64 num, bool word = true) {
                nums.Push(TNum(num, word));
                coords.Push(TCoord(begin - p0, p - p0));
            }

            void OnSep() {
                sep = TCoord(begin - p0, p - p0);
                fail = false;
            }

            void CheckSep() {
                fail = !TCoord::Equals(p0, sep, TCoord(begin - p0, p - p0));
            }

            TDateCoord OnDateDMY() {
                return Check(TDateCoord(MakeDMY(), TCoord(coords[0].Begin, coords[2].End)));
            }

            TDateCoord OnDateMDY() {
                return Check(TDateCoord(MakeMDY(), TCoord(coords[0].Begin, coords[2].End)));
            }

            TDateCoord OnDateYMD() {
                return Check(TDateCoord(MakeYMD(), TCoord(coords[0].Begin, coords[2].End)));
            }

            TDateCoord OnDateYDM() {
                return Check(TDateCoord(MakeYDM(), TCoord(coords[0].Begin, coords[2].End)));
            }

            TDateCoord OnDateMY() {
                return Check(TDateCoord(TDaterDate::MakeDateMonth(nums[1], nums[0]), //
                                        TCoord(coords[0].Begin, coords[1].End)));
            }

            TDateCoord OnDateYM() {
                return Check(TDateCoord(TDaterDate::MakeDateMonth(nums[0], nums[1]), //
                                        TCoord(coords[0].Begin, coords[1].End)));
            }

            TDateCoord OnDate6() {
                return Rollback(TDateCoord(FromDig6(nums[0]), coords[0]));
            }

            TDateCoord OnDate8() {
                return Rollback(TDateCoord(FromDig8More(nums[0]), coords[0]));
            }

            TDateCoord OnDateXMXY() {
                return Check(TDateCoord(MakeXMXY(), TCoord(coords[0].Begin, coords[2].End)));
            }

            TDateCoord OnDateXMXD() {
                return Check(TDateCoord(MakeXMXD(), TCoord(coords[0].Begin, coords[2].End)));
            }

            TDateCoord OnDateY() {
                return Rollback(TDateCoord(TDaterDate::MakeDateYear(nums[0]), coords[0]));
            }

        private:
            TDateCoord Check(TDateCoord d) {
                return Rollback(fail ? TDateCoord() : d);
            }

            TDateCoord Rollback(TDateCoord d) {
                if (!d)
                    return d;

                if (!d.From)
                    d.From = IsUrl ? TDaterDate::FromUrl : TDaterDate::FromText;

                d.WordPattern = nums[0].Word || nums[1].Word && d.Month || nums[2].Word && d.Day;

                if (p < pe)
                    p--; //return last separator.

                return d;
            }

            TDaterDate MakeYMD() {
                return TDaterDate::MakeDateFull(nums[0], nums[1], nums[2]);
            }

            TDaterDate MakeYDM() {
                return TDaterDate::MakeDateFull(nums[0], nums[2], nums[1]);
            }

            TDaterDate MakeDMY() {
                return TDaterDate::MakeDateFull(nums[2], nums[1], nums[0]);
            }

            TDaterDate MakeMDY() {
                return TDaterDate::MakeDateFull(nums[2], nums[0], nums[1]);
            }

            TDaterDate MakeXMXY() {
                TDaterDate d;

                if (!nums[1])
                    return d;

                (d = TDaterDate::MakeDateFull(nums[0], nums[1], nums[2])) //
                    || (d = TDaterDate::MakeDateFull(nums[2], nums[1], nums[0]));
                return d;
            }

            TDaterDate MakeXMXD() {
                TDaterDate d;

                if (!nums[1])
                    return d;

                (d = TDaterDate::MakeDateFull(nums[2], nums[1], nums[0])) //
                    || (d = TDaterDate::MakeDateFull(nums[0], nums[1], nums[2]));
                return d;
            }

            static TDaterDate FromDig8More(ui64 n1) {
                TDaterDate d;

                ui64 n2 = n1;
                while (n2 >= 100000000)
                    n2 /= 10;

                (d = FromDig8(n2)) || (d = FromDig8(n1 % 100000000));

                return d;
            }

            static TDaterDate FromDig(ui64 n1, bool is8) {
                ui32 yearA = n1 / 10000 == 0 && is8 ? -1 : n1 / 10000;
                ui32 monA = (n1 / 100) % 100;
                ui32 dayA = n1 % 100;
                ui32 yearpos = is8 ? 10000 : 100;
                ui32 yearB = n1 % yearpos == 0 && is8 ? -1 : n1 % yearpos;
                ui32 monB = (n1 / yearpos) % 100;
                ui32 dayB = n1 / yearpos / 100;

                TDaterDate d;
                (d = TDaterDate::MakeDateFull(yearA, monA, dayA)) //
                    || (d = TDaterDate::MakeDateFull(yearB, monB, dayB));

                if (IsUrl)
                    d.From = TDaterDate::FromUrlId;

                return d;
            }

            static TDaterDate FromDig8(ui64 n1) {
                return FromDig(n1, true);
            }

            static TDaterDate FromDig6(ui64 n1) {
                return FromDig(n1, false);
            }
        };

        template <typename Char>
        inline bool IsUrlSeparator(Char ch) {
            return !isdigit(ch);
        }

        inline bool IsTextDigSeparator(wchar16 ch) {
            return !IsDigit((wchar32)ch);
        }

        inline bool IsTextYSeparator(wchar16 ch) {
            return !IsDigit((wchar32)ch);
        }

        inline bool IsTextSeparator(wchar16 ch) {
            return !IsAlnum((wchar32)ch);
        }

        TDateCoord UrlDigPattern(const char*& p, const char* pe);
        TDateCoord UrlDigPatternWide(const wchar16*& p, const wchar16* pe);

        TDateCoord UrlWordPattern(const char*& p, const char* pe);
        TDateCoord UrlWordPatternWide(const wchar16*& p, const wchar16* pe);

        TDateCoord HostYPattern(const char*& p, const char* pe);
        TDateCoord UrlXXPattern(const char*& p, const char* pe);
        TDateCoord UrlXXPatternWide(const wchar16*& p, const wchar16* pe);

        TDateCoord TextDigPattern(const wchar16*& p, const wchar16* pe);
        TDateCoord TextWordPattern(const wchar16*& p, const wchar16* pe);
        TDateCoord TextXXPattern(const wchar16*& p, const wchar16* pe);
        TDateCoord TextYPattern(const wchar16*& p, const wchar16* pe);

    }
}
