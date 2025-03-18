#include "cron.h"

#include "log.h"

#include <util/generic/vector.h>

// ------------------------------------------------------------------------

namespace LogSubsystem {
class cron {};
};

// ------------------------------------------------------------------------

enum EConds
{
    COND_VALUE,             // 7
    COND_RANGE,             // 2-11
    COND_RANGE_WITH_STEP,   // 5-17/3
    COND_ANY_WITH_STEP,     // */2
};

class TCond
{
public:
    typedef TVector<ui8> TListType;
    TListType List;

    bool operator()(ui8 n) const {
        if (List.empty())
            return true;

        for (TListType::const_iterator i = List.begin(); i != List.end(); ++i) {
            switch (*i) {
            case COND_VALUE: {
                ui8 val = *(++i);
                if (val == n)
                    return true;
                break;
            }
            case COND_RANGE: {
                ui8 lhs = *(++i);
                ui8 rhs = *(++i);
                if (lhs <= n && n <= rhs)
                    return true;
                break;
            }
            case COND_RANGE_WITH_STEP: {
                ui8 lhs = *(++i);
                ui8 rhs = *(++i);
                ui8 step = *(++i);
                if (lhs <= n && n <= rhs && !(n % step))
                    return true;
                break;
            }
            case COND_ANY_WITH_STEP: {
                ui8 step = *(++i);
                if (!(n % step))
                    return true;
                break;
            }
            default:
                Y_FAIL("Unknown condition id");
            }
        }

        return false;
    }

    bool IsRestricted() const {
        return !List.empty();
    }
};

IOutputStream& operator<<(IOutputStream& os, TCond::TListType::const_iterator &i)
{
    switch (*i) {
    case COND_VALUE: {
        ui8 val = *(++i);
        os << (unsigned)val;
        break;
    }
    case COND_RANGE: {
        ui8 lhs = *(++i);
        ui8 rhs = *(++i);
        os << (unsigned)lhs << '-' << (unsigned)rhs;
        break;
    }
    case COND_RANGE_WITH_STEP: {
        ui8 lhs = *(++i);
        ui8 rhs = *(++i);
        ui8 step = *(++i);
        os << (unsigned)lhs << '-' << (unsigned)rhs << '/' << (unsigned)step;
        break;
    }
    case COND_ANY_WITH_STEP: {
        ui8 step = *(++i);
        os << "*/" << (unsigned)step;
        break;
    }
    default:
        Y_FAIL("Unknown condition id");
    }

    return os;
}

IOutputStream& operator<<(IOutputStream& os, const TCond& obj)
{
    TCond::TListType::const_iterator i = obj.List.begin();

    if (i != obj.List.end()) {
        os << i;
        ++i;
        for (; i != obj.List.end(); ++i) {
            os << ',';
            os << i;
        }
    } else {
        os << '*';
    }
    return os;
}

class TDayOfWeekCond: public TCond
{
public:
    bool operator()(ui8 n) const {
        if (List.empty())
            return true;

        for (TListType::const_iterator i = List.begin(); i != List.end(); ++i) {
            switch (*i) {
            case COND_VALUE: {
                ui8 val = *(++i);
                if (val % 7 == n % 7)
                    return true;
                break;
            }
            case COND_RANGE: {
                ui8 lhs = *(++i);
                ui8 rhs = *(++i);
                if (n % 7) {
                    if (lhs <= n && n <= rhs)
                        return true;
                } else {
                    if (n % 7 == lhs % 7 || n % 7 == rhs % 7)
                        return true;
                }
                break;
            }
            case COND_RANGE_WITH_STEP: {
                ui8 lhs = *(++i);
                ui8 rhs = *(++i);
                ui8 step = *(++i);
                if (!(n % step))
                    return true;
                if (n % 7) {
                    if (lhs <= n && n <= rhs)
                        return true;
                } else {
                    if (n % 7 == lhs % 7 || n % 7 == rhs % 7)
                        return true;
                }
                break;
            }
            case COND_ANY_WITH_STEP: {
                ui8 step = *(++i);
                if (!(n % step))
                    return true;
                break;
            }
            default:
                Y_FAIL("Unknown condition id");
            }
        }

        return false;
    }
};

IOutputStream& operator<<(IOutputStream& os, const TDayOfWeekCond& obj)
{
    os << (TCond)obj;
    return os;
}

static void CheckStep(ui8 step, ui8 max)
{
    if (step <= 1 || step > (max + 1) / 2)
        ythrow TCronException() << "Step " << (unsigned)step << " should be greater than 1 and no greater than " << (unsigned)(max + 1) / 2;
}

static void CheckRange(ui8 lhs, ui8 rhs)
{
    if (lhs >= rhs)
        ythrow TCronException() << "Minimum value " << (unsigned)lhs << " should be less than maximum value " << (unsigned)rhs;
}

#define V(v,n,min,max) \
    do { \
        if(!(min <= v && v <= max)) { \
            ythrow TCronException() << n << " " << (unsigned)v << " isn't within allowed range [" << (unsigned)min << ", " << (unsigned)max << "]"; \
        } \
    } while(0)

class TFieldProcessor
{
private:
    TCond::TListType CurrentCondList;

public:
    void ProcessField(TCond* cond) {
        cond->List = std::move(CurrentCondList);
        CurrentCondList.clear();
    }

    static void Check(const TCond &cond, ui8 min, ui8 max) {
        for (TCond::TListType::const_iterator i = cond.List.begin(); i != cond.List.end(); ++i) {
            switch (*i) {
            case COND_VALUE: {
                ui8 val = *(++i);
                V(val, "Value", min, max);
                break;
            }
            case COND_RANGE: {
                ui8 lhs = *(++i);
                ui8 rhs = *(++i);
                V(lhs, "Minimum value", min, max);
                V(rhs, "Maximum value", min, max);
                CheckRange(lhs, rhs);
                break;
            }
            case COND_RANGE_WITH_STEP: {
                ui8 lhs = *(++i);
                ui8 rhs = *(++i);
                ui8 step = *(++i);
                V(lhs, "Minimum value", min, max);
                V(rhs, "Maximum value", min, max);
                CheckRange(lhs, rhs);
                CheckStep(step, max);
                break;
            }
            case COND_ANY_WITH_STEP: {
                ui8 step = *(++i);
                CheckStep(step, max);
                break;
            }
            default:
                Y_FAIL("Unknown condition id");
            }
        }
    }

    void Value(ui8 val) {
        CurrentCondList.push_back(COND_VALUE);
        CurrentCondList.push_back(val);
    }

    void Range(ui8 lhs, ui8 rhs) {
        CurrentCondList.push_back(COND_RANGE);
        CurrentCondList.push_back(lhs);
        CurrentCondList.push_back(rhs);
    }

    void RangeWithStep(ui8 lhs, ui8 rhs, ui8 step) {
        CurrentCondList.push_back(COND_RANGE_WITH_STEP);
        CurrentCondList.push_back(lhs);
        CurrentCondList.push_back(rhs);
        CurrentCondList.push_back(step);
    }

    void AnyWithStep(ui8 step) {
        CurrentCondList.push_back(COND_ANY_WITH_STEP);
        CurrentCondList.push_back(step);
    }

};

// -----------------------------------------------------------------------

class TCronEntry::TImpl
{
public:
    static TImpl* Create(const TString &entry);
    bool operator()(const TInstant &time) const;
    bool IsCronFormat() const;
private:
    TImpl(const TCond &second,
          const TCond &minute,
          const TCond &hour,
          const TCond &dom,
          const TCond &month,
          const TDayOfWeekCond &dow,
          bool cronFormat)
            :   Second(second),
                Minute(minute),
                Hour(hour),
                DayOfMonth(dom),
                Month(month),
                DayOfWeek(dow),
                CronFormat(cronFormat)
    {
    }

    TCond Second;
    TCond Minute;
    TCond Hour;
    TCond DayOfMonth;
    TCond Month;
    TDayOfWeekCond DayOfWeek;
    bool CronFormat;

    friend IOutputStream& operator<<(IOutputStream& os, const TCronEntry::TImpl& obj);
};

bool TCronEntry::TImpl::IsCronFormat() const
{
    return CronFormat;
}

bool TCronEntry::IsCronFormat() const
{
    return Impl->IsCronFormat();
}

bool TCronEntry::TImpl::operator()(const TInstant &time) const
{
    struct tm now;
    time.LocalTime(&now);
    bool triggered = Minute(now.tm_min) &&
                     Hour(now.tm_hour) &&
                     Month(now.tm_mon + 1) &&
                     ( ( DayOfMonth.IsRestricted() && DayOfMonth(now.tm_mday)) ||
                       ( DayOfWeek.IsRestricted() && DayOfWeek(now.tm_wday ? now.tm_wday : 7) ) ||
                       ( ! DayOfMonth.IsRestricted() && ! DayOfWeek.IsRestricted() ) );
    if (!CronFormat) {
        triggered = triggered && Second(now.tm_sec);
    }

    DEBUGLOG1(cron, "`" << *this << "' is triggered: " << triggered);

    return triggered;
}

IOutputStream& operator<<(IOutputStream& os, const TCronEntry::TImpl& obj) {
    if (!obj.CronFormat) {
        os << obj.Second << ' ';
    }
    os << obj.Minute << ' ' <<
          obj.Hour << ' ' <<
          obj.DayOfMonth << ' ' <<
          obj.Month << ' ' <<
          obj.DayOfWeek;
    return os;
}

// ------------------------------------------------------------------------

TCronEntry::TCronEntry(const TString &entry)
: Impl(TImpl::Create(entry))
{
}

TCronEntry::~TCronEntry()
{
}

bool TCronEntry::operator()(const TInstant &time) const
{
    return (*Impl)(time);
}

IOutputStream& operator<<(IOutputStream& os, const TCronEntry& obj)
{
    os << *obj.Impl;
    return os;
}
