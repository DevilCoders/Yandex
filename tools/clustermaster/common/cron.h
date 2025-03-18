#pragma once

// From FreeBSD's crontab(5):
//
// ...
//   Commands are executed by cron(8) when the minute, hour,
//   and month of year fields match the current time, and when at least one of
//   the two day fields (day of month, or day of week) matches the current
//   time (see ``Note'' below).  cron(8) examines cron entries once every
//   minute.  The time and date fields are:
//
//         field         allowed values
//         -----         --------------
//         minute        0-59
//         hour          0-23
//         day of month  1-31
//         month         1-12 (or names, see below)
//         day of week   0-7 (0 or 7 is Sun, or use names)
//
//   A field may be an asterisk (*), which always stands for ``first-last''.
//
//   Ranges of numbers are allowed.  Ranges are two numbers separated with a
//   hyphen.  The specified range is inclusive.  For example, 8-11 for an
//   ``hours'' entry specifies execution at hours 8, 9, 10 and 11.
//
//   Lists are allowed.  A list is a set of numbers (or ranges) separated by
//   commas.  Examples: ``1,2,5,9'', ``0-4,8-12''.
//
//   Step values can be used in conjunction with ranges.  Following a range
//   with ``/<number>'' specifies skips of the number's value through the
//   range.  For example, ``0-23/2'' can be used in the hours field to specify
//   command execution every other hour (the alternative in the V7 standard is
//   ``0,2,4,6,8,10,12,14,16,18,20,22'').  Steps are also permitted after an
//   asterisk, so if you want to say ``every two hours'', just use ``*/2''.
//
// ...
//   Note: The day of a command's execution can be specified by two fields —
//   day of month, and day of week.  If both fields are restricted (ie, are
//   not *), the command will be run when either field matches the current
//   time.  For example, ``30 4 1,15 * 5'' would cause a command to be run at
//   4:30 am on the 1st and 15th of each month, plus every Friday.

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/stream/output.h>

class TCronEntry
{
public:
    TCronEntry(const TString &entry);
    ~TCronEntry();
    bool operator()(const TInstant &time = TInstant::Now()) const;
    bool IsCronFormat() const;

private:
    class TImpl;
    THolder<TImpl> Impl;

    friend IOutputStream& operator<<(IOutputStream& os, const TCronEntry& obj);
    friend IOutputStream& operator<<(IOutputStream& os, const TCronEntry::TImpl& obj);
};

class TCronException: public yexception {};
