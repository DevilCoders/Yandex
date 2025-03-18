# -*- coding: utf-8 -*-

from urlparse import urljoin

from django.utils.translation import ugettext_lazy as _

from core.actions.common import jsonify
from core.settings import AVATARS_URL
from core.templatetags.hidereferer import hidereferer


class JsonFields:
    HILITEDURL = 'hilitedurl'
    URLMENU = 'urlmenu'
    HEADLINE = 'headline'
    BY_LINK = 'by_link'
    HEADLINE_SRC = 'headline_src'
    MARKET = 'market'
    SPEC_ATTRS = 'spec_attrs'
    LISTDATA = 'listData'
    IMG_AVATARS = 'img_avatars'
    IMG = 'img'
    LINK_ATTRS = 'link_attrs'


def parse_by_link_info(by_link):
    return by_link

def parse_img(imgs):
    return imgs

def parse_img_avatars(imgs):
    return [urljoin(AVATARS_URL, url) for url in imgs]

def parse_urlmenu_info(urlmenu):
    return urlmenu


def parse_market_info(market_str):
    market_dict = dict(pair.split('=', 1)
                       for pair in market_str.split(u'\u0007;'))
    return market_dict


def parse_spec_attrs_info(attrs_str):
    attrs_dict = {
        k: jsonify(v, 'JSONIFY_SPEC_ATTRS_LOAD_ERROR') for k, v in zip(
            *[iter(attrs_str.split('\t'))] * 2
        )
    }
    return attrs_dict


def parse_link_attrs(link_attrs_str):
    return [hidereferer(pair.split('=', 1)[1]) \
            for pair in link_attrs_str.split(u'\u0007;') \
            if pair.startswith('url=')]


def parse(extra_info):
    extra_res = {}

    if JsonFields.BY_LINK in extra_info:
        extra_res[JsonFields.BY_LINK] = \
            parse_by_link_info(extra_info[JsonFields.BY_LINK])
    if JsonFields.URLMENU in extra_info:
        extra_res[JsonFields.URLMENU] = \
            parse_urlmenu_info(extra_info[JsonFields.URLMENU])
    if JsonFields.MARKET in extra_info:
        extra_res[JsonFields.MARKET] = \
            parse_market_info(extra_info[JsonFields.MARKET])
    if JsonFields.SPEC_ATTRS in extra_info:
        extra_res[JsonFields.SPEC_ATTRS] = \
            parse_spec_attrs_info(extra_info[JsonFields.SPEC_ATTRS])
    if JsonFields.IMG in extra_info:
        extra_res[JsonFields.IMG] = \
            parse_img(extra_info[JsonFields.IMG])
    if JsonFields.IMG_AVATARS in extra_info:
        extra_res[JsonFields.IMG_AVATARS] = \
            parse_img_avatars(extra_info[JsonFields.IMG_AVATARS])
    if JsonFields.LINK_ATTRS in extra_info:
        extra_res[JsonFields.LINK_ATTRS] = \
            parse_link_attrs(extra_info[JsonFields.LINK_ATTRS])
    return extra_res


# info is taken from "https://github.yandex-team.ru/search-interfaces/common/blob/master/blocks-common/i-global/__currency/i-global__currency.priv.js"
MARKET_CURRENCIES = {
    'RUR': _(u'rub.'),
    'EUR': _(u'€'),
    'USD': _(u'$'),
    'UAH': _(u'₴'),
    'KZT': _(u'tenge'),
    'BYR': _(u'Belarusian rubles'),
    'TRY': _(u'TL'),
}


def prepare_market_info(market_dict):
    currency = MARKET_CURRENCIES.get(market_dict['currency'])
    if currency:
        market_dict['currency'] = currency
    return market_dict


def prepare_spec_attrs_info(attrs_dict):
    if JsonFields.LISTDATA in attrs_dict:
        data = attrs_dict[JsonFields.LISTDATA]
        if isinstance(data, list) and data:
            attrs_dict[JsonFields.LISTDATA] = data[0]
        else:
            attrs_dict[JsonFields.LISTDATA] = {}
        if not isinstance(attrs_dict[JsonFields.LISTDATA].get('if'), int):
            attrs_dict[JsonFields.LISTDATA]['if'] = None
        if not isinstance(attrs_dict[JsonFields.LISTDATA].get('il'), int):
            attrs_dict[JsonFields.LISTDATA]['il'] = None
    return attrs_dict


def prepare(extra_info):
    if JsonFields.MARKET in extra_info:
        extra_info[JsonFields.MARKET] = \
            prepare_market_info(extra_info[JsonFields.MARKET])
    if JsonFields.SPEC_ATTRS in extra_info:
        extra_info[JsonFields.SPEC_ATTRS] = \
            prepare_spec_attrs_info(extra_info[JsonFields.SPEC_ATTRS])

    return extra_info
