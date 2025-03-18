import re
import time
import time_utl

from antirobot.scripts.utils import ip_utils

ip4DigitsRe = re.compile(r"^\d{1,3}$")
ip6GroupRe = re.compile(r"^[0-9a-fA-F]{0,4}$")


def validate_ip4(ipStr):
    fields = ipStr.split(".");
    if len(fields) < 3 or len(fields) > 4:
        return False;

    for f in fields:
        m = ip4DigitsRe.match(f);
        if not m:
            return False;

        try:
            a = int(f);
            if a < 0 or a > 255:
                return False;
        except:
            return False;

    return True;


def validate_ip6(ipStr):
    fields = ipStr.split(':')

    if len(fields) < 3 or len(fields) > 8:
        return False

    for f in fields:
        if len(f) > 4:
            return False

        if not ip6GroupRe.match(f):
            return False

    return True


def validate_ip(ip):
    if not ip:
        return False;

    if ip.find('.') >= 0:
        return validate_ip4(ip)
    else:
        return validate_ip6(ip)


def validate_date(date):
    if not date:
        return None;

    usualDateRe = re.compile(r"^(\d{2})\.(\d{2})\.(\d{4})$");
    m = usualDateRe.match(date);
    if m:
        date = "%s%s%s" % (m.group(3), m.group(2), m.group(1));

    dateRe = re.compile(r"^\d{8}$");
    if not dateRe.match(date):
        return None;

    try:
        year = int(date[:4]);
        if year < 2008 or year > 2100:
            return None;

        month = int(date[4:6]);
        if month < 1 or month > 12:
            return None;

        day = int(date[6:8]);
        if day < 1 or day > 31:
            return None;
    except:
        return None;

    if date > time.strftime("%Y%m%d"):
        return None;

    return date;

def validate_ctime(ctime):
    try:
        tim = int(ctime)
        if ctime > 0:
            return tim
    except:
        return None


def validate_time(timeStr):
    try:
        return time_utl.TimeFromStr(timeStr)
    except:
        return None

def validate_ctimestr(time_str):
    try:
        return int(time_str)
    except:
        return None
