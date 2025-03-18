#pragma once

#include <library/cpp/packedtypes/longs.h>

#include <cstdlib>
#include <cstring>
#include <algorithm>

class TStringComparer {

public:

    void Check(const char* text, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            if (Pos == -1)
                return;
            if (Str[Pos] == 0) {
                Pos = -1; 
            } else if (Str[Pos] == text[i]) {
                ++Pos;
            } else {
                Pos = -1;
            }
        }
    }

    void Reset() {
        Pos = 0;
    }

    bool Result() const {
        return ((size_t)Pos == Len);
    }

    TStringComparer(const char* const str)
        : Str(str)
        , Len(strlen(str))
    {
        Reset();
    }

private:

    int Pos;
    const char* const Str;
    const size_t Len;

};

class TUnsignedParser {

public:

    bool Parse(const char* text, size_t len) {
        for (unsigned i = 0; i < len; ++i, ++text) {
	    if (((*text) == '\t') || ((*text) == ' ') || ((*text) == '\r') || ((*text) == '\n'))
	        continue;
	    if (((*text) < '0') || ((*text) > '9')) {
	        Init();
	        return false;
	    }
	    Value = Value * 10 + ((*text) - '0');
	}
        return true;
    }

    unsigned Result() {
        return Value;
    }
    
    void Init() {
        Value = 0;
    }
    
    TUnsignedParser() {
        Init();
    }

private:

    unsigned Value;

};


class TFloatParser {

public:

    bool Parse(const char* text, size_t len) {
        for (unsigned i = 0; i < len; ++i, ++text) {
	    if (((*text) == '\t') || ((*text) == ' ') || ((*text) == '\r') || ((*text) == '\n'))
	        continue;
            if ((*text) == '.') {
                if (Exp != 0) {
                    Init();
                    return false;
                }
                Exp = 1;
                continue;
            }
	    if (((*text) < '0') || ((*text) > '9')) {
	        Init();
	        return false;
	    }
	    Value = Value * 10 + ((*text) - '0');
            if (Exp) Exp /= 10.0f;
	}
        return true;
    }

    double Result() {
        return Exp ? (double)Value * Exp : Value;
    }

    void Init() {
        Value = 0;
        Exp = 0;
    }

    TFloatParser() {
        Init();
    }

private:

    ui64 Value;
    double Exp;

};


class TBoolParser {

        %%{
            machine ParseBool;
            alphtype char;

            yes = "Yes" | "YES" | "yes" 
                | "True" | "TRUE" | "true"
                | "On" | "ON" | "on"
                | "1";

            no  = "No" | "NO" | "no"
                | "False" | "FALSE" | "false"
                | "Off" | "OFF" | "off"
                | "0";
            
            main := yes @{ m_result = 1; fbreak; }
                  | no  @{ m_result = -1; fbreak; }
                  $!{ return false; };

        }%%
 
public:

    bool Parse(const char* text, size_t len) {
        const char* p = text;
        const char* pe = text + len;
        %%write data noerror nofinal;
        %%write exec;
        return true;
        (void) ParseBool_start;
        (void) ParseBool_en_main;
    }

    int Result() const {
        return m_result;
    }

    void Init() {
        %%write data noerror nofinal;
        %%write init;
        m_result = 0;
        (void) ParseBool_start;
        (void) ParseBool_en_main;
    }

    TBoolParser() {
        Init();
    }

private:

    int cs;
    int m_result;

};


class TDateTimeParserBaseDeprecatedOld {

public:

    time_t Result() const {
        const unsigned monthdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (yearpart < 1970 || monthpart < 1 || monthpart > 12)
            return 0;
        {
            unsigned day = datetimepart / 86400 + 1;
            unsigned md = monthdays[monthpart - 1];
	    if ((monthpart == 2) && (yearpart % 4 == 0))
	        md += 1;
	    if (day > md) {
                return 0;
	    }
        }
        time_t ofspart = negative ? -offsetpart : offsetpart;
        time_t dtpart = datetimepart;
        dtpart += (365 * 86400) * (yearpart - 1970);
        dtpart += 86400 * ((yearpart - 1969) / 4);
        for (unsigned i = 0; i < monthpart - 1; ++i) {
            dtpart += 86400 * monthdays[i];
        }
        if ((monthpart > 2) && (yearpart % 4 == 0)) {
            dtpart += 86400;
        }
        time_t res = dtpart - ofspart;
        return res;
    }

    void Init() {
        yearpart = monthpart = 0;
        datetimepart = 0;
        offsetpart = 3 * 3600; // Moscow time by default
        negative = false;
    }

    TDateTimeParserBaseDeprecatedOld() {
        Init();
    }

protected:

    int cs;

    unsigned int anum;
    unsigned int yearpart;
    unsigned int monthpart;
    time_t datetimepart;
    time_t offsetpart;
    bool negative; 

};

class TDateTimeParser : public TDateTimeParserBaseDeprecatedOld {

        %%{
            machine ParseDateTime;
            alphtype char;

            action st { anum = 0; }
            action sv { anum = anum * 10 + (*p - '0'); }

            hh = ([01] @sv '0'..'9' @sv ) | ('2' @sv '0'..'3' @sv );
            mmss = ('0'..'5' @sv '0'..'9' @sv);
            yyyy = (('1' @sv '9' @sv '7'..'9' @sv) | ('2' @sv '0' @sv '0'..'2' @sv)) '0'..'9' @sv;
            mm = ('0' @sv '1'..'9' @sv) | ('1' @sv '0'..'2' @sv);
            dd = ('0'..'2' @sv '0'..'9' @sv) | ('3' @sv [01] @sv);
            dash = ('-' | '.');

            hour = hh           >st @{ datetimepart += 3600 * anum; };
            minute = mmss       >st @{ datetimepart += 60 * anum; };
            second = mmss       >st @{ datetimepart += anum; };
            hourdelta = hh      >st @{ offsetpart = 3600 * anum; };
            minutedelta = mmss  >st @{ offsetpart += 60 * anum; };
            year = yyyy         >st @{ yearpart = anum; };
            month = mm          >st @{ monthpart = anum; };
            day = dd            >st @{ datetimepart = 86400 * (anum - 1);} ;

            sign = ('+' >{ negative = false; } | '-' >{ negative = true; });
            offset = ([Zz] >{ offsetpart = 0; } ) | (sign hourdelta ":"? minutedelta);
            date = year dash month dash day;
            time = [Tt ] hour ":" minute ":" second ("." ('0'..'9')+)? offset?;
            
            main := (date time?) $!{ return false; };

        }%%
 
public:

    bool Parse(const char* text, size_t len) {
        const char* p = text;
        const char* pe = text + len;
        %%write data noerror nofinal;
        %%write exec;
        (void) ParseDateTime_start;
        (void) ParseDateTime_en_main;
        return true;
    }

    void Init() {
        %%write data noerror nofinal;
        %%write init;
        (void) ParseDateTime_start;
        (void) ParseDateTime_en_main;
        TDateTimeParserBaseDeprecatedOld::Init();
    }
    
    TDateTimeParser() {
        Init();
    }

};

class TDateTimeParserRFC822 : public TDateTimeParserBaseDeprecatedOld {

        %%{
            machine ParseDateTimeRFC822;
            alphtype char;

            action st { anum = 0; }
            action sv { anum = anum * 10 + (*p - '0'); }

            hh = ([01] @sv '0'..'9' @sv ) | ('2' @sv '0'..'3' @sv );
            mmss = ('0'..'5' @sv '0'..'9' @sv);
            yyyy = (('1' @sv '9' @sv '7'..'9' @sv) | ('2' @sv '0' @sv '0'..'2' @sv)) '0'..'9' @sv;
            mm = ('0' @sv '1'..'9' @sv) | ('1' @sv '0'..'2' @sv);
            dd = ('0'..'2' @sv ('0'..'9' @sv)?) | ('3' @sv ([01] @sv)?) | ('4'..'9' @sv);

            hour = hh           >st @{ datetimepart += 3600 * anum; };
            minute = mmss       >st @{ datetimepart += 60 * anum; };
            second = mmss       >st @{ datetimepart += anum; };
            hourdelta = hh      >st @{ offsetpart = 3600 * anum; };
            minutedelta = mmss  >st @{ offsetpart += 60 * anum; };
            year = yyyy         >st @{ yearpart = anum; };
            day = dd            >st @{ datetimepart = 86400 * (anum - 1);} ;

            month = "Jan" @{ monthpart = 1; }
                  | "Feb" @{ monthpart = 2; }
                  | "Mar" @{ monthpart = 3; }
                  | "Apr" @{ monthpart = 4; }
                  | "May" @{ monthpart = 5; }
                  | "Jun" @{ monthpart = 6; }
                  | "Jul" @{ monthpart = 7; }
                  | "Aug" @{ monthpart = 8; }
                  | "Sep" @{ monthpart = 9; }
                  | "Oct" @{ monthpart = 10; }
                  | "Nov" @{ monthpart = 11; }
                  | "Dec" @{ monthpart = 12; };

            sign = ('+' >{ negative = false; } | '-' >{ negative = true; });

            offset = "Z" @{ offsetpart = 0; }
                   | "A" @{ offsetpart = -3600; }
                   | "M" @{ offsetpart = -12 * 3600; }
                   | "N" @{ offsetpart = 3600; }
                   | "Y" @{ offsetpart = 12 * 3600; }
                   | "UT"  @{ offsetpart = 0; }
                   | "GMT" @{ offsetpart = 0; }
                   | "EST" @{ offsetpart = -5 * 3600; }
                   | "EDT" @{ offsetpart = -4 * 3600; }
                   | "CST" @{ offsetpart = -6 * 3600; }
                   | "CDT" @{ offsetpart = -5 * 3600; }
                   | "MST" @{ offsetpart = -7 * 3600; }
                   | "MDT" @{ offsetpart = -6 * 3600; }
                   | "PST" @{ offsetpart = -8 * 3600; }
                   | "PDT" @{ offsetpart = -7 * 3600; }
                   | (sign hourdelta ":"? minutedelta );

            date = day " " month " " year;
            time =  hour ":" minute (":" second)? (" "? offset)?;
            
            weekday = "Mon" | "Tue" |  "Wed" | "Thu" | "Fri" | "Sat" | "Sun";

            main := ((weekday ", ")? date (" " time)?) $!{ return false; };

        }%%
 
public:

    bool Parse(const char* text, size_t len) {
        const char* p = text;
        const char* pe = text + len;
        %%write data noerror nofinal;
        %%write exec;
        (void) ParseDateTimeRFC822_start;
        (void) ParseDateTimeRFC822_en_main;
        return true;
    }

    void Init() {
        %%write data noerror nofinal;
        %%write init;
        (void) ParseDateTimeRFC822_start;
        (void) ParseDateTimeRFC822_en_main;
        TDateTimeParserBaseDeprecatedOld::Init();
    }
    
    TDateTimeParserRFC822() {
        Init();
    }

};
