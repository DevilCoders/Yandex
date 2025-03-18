# coding=utf-8
"""
do some work with url
"""
import json
from urlparse import parse_qsl
from urllib import urlencode
from enum import IntEnum


def url_add_param(url, param):
    return url._replace(query=str(param) + (('&' + url.query) if url.query else ''))


def url_add_query_param(url, param, value, overwrite=False):
    """
    Adding query param with arg to current url.
    :param url: url (result of `urlparse.urparse()`)
    :param param: param to be added to url
    :param value: value of query param
    :param overwrite: if True - delete all existing query params with `param` name
    :return: url with `&param=value` query
    """
    query_params = parse_qsl(url.query)
    if overwrite:
        query_params = [(p, a) for p, a in query_params if p != param]
    query_params.append((param, value))
    return url._replace(query=urlencode(query_params))


def url_delete_query_params(url, params):
    """
    Delete query params from url
    :param url: url (result of `urlparse.urparse()`)
    :param params: list with params to delete from url
    :return: url without params from `params` list
    """
    query_params = [(p, a) for p, a in parse_qsl(url.query) if p not in params]
    return url._replace(query=urlencode(query_params))


def url_keep_query_params(url, params):
    """
    Keep only query params from url
    """
    query_params = [(p, a) for p, a in parse_qsl(url.query) if p in params]
    return url._replace(query=urlencode(query_params))


def url_keep_json_keys_in_query_params(url, settings):
    """
    settings: dict()
    key = param_name, value = list() of keys to keep in url
    """
    query_params = parse_qsl(url.query)
    settings_params = [(p, a) for p, a in query_params if p in settings]
    result = [(p, a) for p, a in query_params if p not in settings]
    for param, value in settings_params:
        try:
            param_json = json.loads(value)
            param_json = sorted([(p, a) for p, a in param_json.items() if p in settings[param]])
            result.append((param, json.dumps(param_json)))
        except ValueError:
            continue
    return url._replace(query=urlencode(result))


class UrlClass(IntEnum):
    """
    Simple URL classes
    """

    (ADFOX,
     BK,
     BK_META_AUCTION,
     BK_META_BULK_AUCTION,
     BK_VMAP_AUCTION,
     RTB_AUCTION,
     RTB_COUNT,
     YABS_COUNT,
     YANDEX,
     STATICMON,
     PCODE,
     PCODE_LOADER,
     RAMBLER,
     RAMBLER_AUCTION,
     PARTNER,
     COUNTERS,
     YANDEX_METRIKA,
     XHR_PROTECTION,
     SCRIPT_PROTECTION,
     AUTOREDIRECT_SCRIPT,
     NANPU,
     NANPU_STARTUP,
     NANPU_AUCTION,
     NANPU_BK_AUCTION,
     NANPU_COUNT,
     STRM,
     DZEN_PLAYER,
     DENY) = range(28)


# пока поддержим посты только для партнерских и count урлов, а также для запросов INAPP и балковых рекламных запросов
TAGS_ALLOWED_POST_REQUESTS = [UrlClass.PARTNER, UrlClass.RTB_COUNT, UrlClass.NANPU, UrlClass.BK_META_BULK_AUCTION, UrlClass.YANDEX_METRIKA]


def classify_url(config, urlstr):
    """
    Helping method to classify url
    TODO: Нужно переписать этот метод с учетом включенных рекламных платформ. Логика с DENY ссылками должна уехать в другое место

    :param config: cryprox config object
    :param urlstr: url to classify
    :return: list with url classes
    """

    def append_bk_adfox_classes(urlstr, classes):
        if config.adfox_url_re_match.match(urlstr):
            classes.append(UrlClass.ADFOX)
        if config.bk_url_re_match.match(urlstr):
            classes.append(UrlClass.BK)

    classes = list()

    if config.nanpu_url_re_match.match(urlstr):
        classes += [UrlClass.YANDEX, UrlClass.NANPU]
        if config.nanpu_auction_url_re_match.match(urlstr):
            classes.append(UrlClass.NANPU_AUCTION)
        elif config.nanpu_startup_url_re_match.match(urlstr):
            classes.append(UrlClass.NANPU_STARTUP)
        elif config.nanpu_count_url_re_match.match(urlstr):
            classes.append(UrlClass.NANPU_COUNT)
        return classes
    if config.nanpu_bk_auction_url_re_match.match(urlstr):
        classes += [UrlClass.YANDEX, UrlClass.NANPU, UrlClass.NANPU_AUCTION, UrlClass.NANPU_BK_AUCTION]
        return classes
    if config.rtb_count_url_re_match.match(urlstr):
        classes += [UrlClass.YANDEX, UrlClass.RTB_COUNT]
        if config.yabs_count_url_re_match.match(urlstr):
            classes += [UrlClass.YABS_COUNT, UrlClass.BK]
            return classes
        append_bk_adfox_classes(urlstr, classes)
        return classes
    if config.rtb_auction_url_re_match.match(urlstr):
        classes += [UrlClass.YANDEX, UrlClass.RTB_AUCTION]
        if config.bk_auction_meta_url_re_match.match(urlstr):
            classes += [UrlClass.BK_META_AUCTION, UrlClass.BK]
            return classes
        append_bk_adfox_classes(urlstr, classes)
        return classes
    if config.detect_lib_url_re_match.match(urlstr):
        classes += [UrlClass.STATICMON, UrlClass.YANDEX, UrlClass.PCODE]
        return classes
    if config.pcode_loader_url_re_match.match(urlstr):
        classes += [UrlClass.PCODE_LOADER, UrlClass.PCODE, UrlClass.YANDEX, UrlClass.BK]
        return classes
    if config.turbo_redirect_script_re_match.match(urlstr):
        classes += [UrlClass.AUTOREDIRECT_SCRIPT, UrlClass.PCODE]
        return classes
    if config.bk_vmap_auction_url_re_match.match(urlstr):
        classes += [UrlClass.BK_VMAP_AUCTION, UrlClass.BK, UrlClass.YANDEX]
        return classes
    if config.bk_auction_meta_bulk_url_re_match.match(urlstr):
        classes += [UrlClass.BK_META_BULK_AUCTION, UrlClass.BK, UrlClass.YANDEX]
        return classes
    if config.pcode_js_url_re_match.match(urlstr):
        classes.append(UrlClass.PCODE)
    if config.force_yandex_url_re_match.match(urlstr):
        classes.append(UrlClass.YANDEX)
        return classes
    if config.strm_url_re_match.match(urlstr):
        classes += [UrlClass.YANDEX, UrlClass.STRM]
        return classes
    if config.dzen_player_url_re_match.match(urlstr):
        classes.append(UrlClass.DZEN_PLAYER)
        return classes
    if config.yandex_metrika_url_re_match.match(urlstr):
        classes.append(UrlClass.YANDEX_METRIKA)
    if config.partner_url_re_match.match(urlstr):
        classes.append(UrlClass.PARTNER)
        return classes
    if config.xhr_protection_fix_style_src_re_match.match(urlstr):
        classes.append(UrlClass.XHR_PROTECTION)
        return classes
    if config.script_protection_fix_style_src_re_match.match(urlstr):
        classes.append(UrlClass.SCRIPT_PROTECTION)
        return classes
    if config.yandex_url_re_match.match(urlstr):
        classes.append(UrlClass.YANDEX)
        append_bk_adfox_classes(urlstr, classes)
        return classes
    if config.bk_counters_url_re_match.match(urlstr):
        classes.append(UrlClass.COUNTERS)
        return classes
    if config.rambler_url_re_match.match(urlstr):
        classes.append(UrlClass.RAMBLER)
        if config.rambler_auction_url_re_match.match(urlstr):
            classes.append(UrlClass.RAMBLER_AUCTION)
        return classes

    return [UrlClass.DENY]
