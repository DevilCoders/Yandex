import itertools
import logging
from functools import total_ordering

import yt.yson

import yaqutils.misc_helpers as umisc
import yaqutils.url_helpers as uurl


def parse_tpah_log(tpah_log):
    """
    :type tpah_log: str | None
    :rtype: list[TpahLogRecord]
    """
    if tpah_log is None:
        return None
    try:
        unquoted_tpah = uurl.unquote(tpah_log).strip()
        return list(iter_tpah_log(unquoted_tpah))
    except AssertionError:
        logging.error("skip bad tpah_log field: %s", tpah_log)
        return None
    except:
        logging.error("bad tpah_log field: %s", tpah_log)
        raise


def iter_tpah_log(tpah_log):
    """
    :type tpah_log: str
    :rtype: __generator[str]
    """
    if not tpah_log:
        return
    assert tpah_log[0] == "["
    assert tpah_log[-1] == "]"
    left = tpah_log.find("[", 1)
    while left >= 0:
        right = tpah_log.find("]", left)
        assert right > left
        raw_part = tpah_log[left + 1:right].strip()
        parsed_part = parse_tpah_part(raw_part)
        yield parsed_part
        left = tpah_log.find("[", right)


def parse_tpah_part(tpah_part):
    """
    :type tpah_part: str
    """
    elements = tpah_part.split(",")
    if len(elements) != 3:
        return [tpah_part]
    action, pos, milliseconds = elements
    return [action.strip(), pos.strip(), umisc.optional_int(milliseconds, milliseconds)]


def prepare_suggest_data(item):
    suggest = item.GetSuggest()
    if not suggest:
        return None
    return {
        "TimeSinceFirstChange": suggest.TimeSinceFirstChange,
        "TimeSinceLastChange": suggest.TimeSinceLastChange,
        "UserKeyPressesCount": suggest.UserKeyPressesCount,
        "UserInput": suggest.UserInput,
        "TpahLog": parse_tpah_log(suggest.TpahLog),
        "Status": suggest.Status,
        "TotalInputTime": suggest.TotalInputTimeInMilliseconds,
    }


def fix_suggest_for_single_query(requests):
    """
    :type requests: list[dict]
    """
    first_page_request = None
    for request in requests:
        page = request.get("page")
        suggest = request.get("suggest")
        if page == 0:
            first_page_request = request
        elif (isinstance(page, str) or isinstance(page, int) and page > 0) and suggest:
            request["suggest"] = None
            if first_page_request and not first_page_request.get("suggest"):
                first_page_request["suggest"] = suggest


@total_ordering
class MinType(object):
    def __le__(self, other):
        return True

    def __eq__(self, other):
        return self is other


NONE = MinType()


def fix_suggest(actions):
    """
    :type actions: list[dict]
    """
    requests = (
        action
        for action in actions
        if action.get("type") == "request" and yt.yson.is_unicode(action.get("query")) and action.get("query")
    )
    for _, group in itertools.groupby(requests, key=lambda r: r["query"]):
        fix_suggest_for_single_query(sorted(group, key=lambda r: (r["ts"], r.get("page") or NONE)))
