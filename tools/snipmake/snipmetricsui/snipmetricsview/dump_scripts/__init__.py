#!/usr/bin/env python
# -*- coding: utf-8 -*-


def _restorePackageStructure():
    from os.path import dirname, sep, pardir
    from sys import path
    path.insert(0, sep.join([dirname(__file__) + "/../.."]))
    path.insert(0, sep.join([dirname(__file__) + "/.."]))
    path.insert(0, sep.join([dirname(__file__)] + [pardir]))
    path.insert(0, sep.join([dirname(__file__), "serp_downloader"]))
