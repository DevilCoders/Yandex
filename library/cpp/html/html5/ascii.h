#pragma once

#include <stddef.h>

#include <string.h>

#include <contrib/libs/libc_compat/string.h>

#include <util/system/defaults.h>

namespace NHtml5 {
    /**
     */
    class TByteIterator {
    public:
        inline TByteIterator(const char* source, size_t source_length)
            : Start_(source)
            , Mark_(nullptr)
            , End_(source + source_length)
        {
            Current_ = (Start_ < End_) ? (unsigned char)(*Start_) : -1;
        }

        // Returns replacement codepoint for invalid input char.
        inline static int ReplacementChar() {
            // Be careful when change replacement char.
            // It can change extracted token type
            // and causes of undefined parser behaviour.
            return ' ';
        }

        // Returns the current byte as an integer.
        inline int Current() const {
            return Current_;
        }

        // Retrieves a character pointer to the start of the current character.
        inline const char* GetCharPointer() const {
            return Start_;
        }

        // Returns true if this Unicode code point is in the list of characters
        // forbidden by the HTML5 spec, such as NUL bytes and undefined control chars.
        inline bool IsInvalidCodepoint(int c) const {
            return (c >= 0x1 && c <= 0x8) || c == 0xB || (c >= 0xE && c <= 0x1F);
        }

        // Advances the current position by one byte.
        inline void Next() {
            if (Current_ == -1) {
                // If we're already at EOF, bail out before advancing anything to avoid
                // reading past the end of the buffer.  It's easier to catch this case here
                // than litter the code with lots of individual checks for EOF.
                return;
            }
            ++Start_;
            Current_ = (Start_ < End_) ? (unsigned char)(*Start_) : -1;
        }

        // If the upcoming text in the buffer matches the specified prefix (which has
        // length 'length'), consume it and return true.  Otherwise, return false with
        // no other effects.  If the length of the string would overflow the buffer,
        // this returns false.  Note that prefix should not contain null bytes because
        // of the use of strncmp/strncasecmp internally.  All existing use-cases adhere
        // to this.
        inline bool MaybeConsumeMatch(const char* prefix, size_t length, bool case_sensitive) {
            bool matched = (Start_ + length <= End_) && (case_sensitive ? !strncmp(Start_, prefix, length) : !strnicmp(Start_, prefix, length));
            if (matched) {
                for (size_t i = 0; i < length; ++i) {
                    Next();
                }
                return true;
            } else {
                return false;
            }
        }

        // "Marks" a particular location of interest in the input stream, so that it can
        // later be reset() to.  There's also the ability to record an error at the
        // point that was marked, as oftentimes that's more useful than the last
        // character before the error was detected.
        inline void Mark() {
            Mark_ = Start_;
        }

        // Returns the current input stream position to the mark.
        inline void Reset() {
            Start_ = Mark_;
            Current_ = (unsigned char)(*Mark_);
        }

    private:
        // Points at the start of the code point most recently read into 'current'.
        const char* Start_;

        // Points at the mark.  The mark is initially set to the beginning of the
        // input.
        const char* Mark_;

        // Points past the end of the iter, like a past-the-end iterator in the STL.
        const char* End_;

        // The byte under the cursor.
        int Current_;
    };
}
