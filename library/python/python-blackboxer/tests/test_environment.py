# -*- encoding: utf-8 -*-
from __future__ import unicode_literals

from blackboxer.environment import ENV, URL


def test_url():
    assert URL == ENV['development']
