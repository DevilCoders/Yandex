# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from pprint import pprint
from ids.registry import registry


detect = registry.get_repository('uatraits', 'detect', user_agent='example')


browser_detect = detect.get_one({
    'data': '{"User-Agent":"Mozilla/5.0 (Windows NT 6.1; WOW64; rv:40.0) Gecko/20100101 Firefox/40.1"}'
})
pprint(browser_detect)


browser_detect = detect.get_one({
    'user_agent': 'Mozilla/5.0 (Windows NT 6.1; WOW64; rv:40.0) Gecko/20100101 Firefox/40.1'
})
pprint(browser_detect)
