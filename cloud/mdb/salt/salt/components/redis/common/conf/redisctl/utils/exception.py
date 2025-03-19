#!/usr/bin/env python
# -*- coding: utf-8 -*-


def get_msg(exc):
    return getattr(exc, 'message', '')
