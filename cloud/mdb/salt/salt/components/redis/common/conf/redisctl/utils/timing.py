#!/usr/bin/env python
# -*- coding: utf-8 -*-
from time import time


def time_left(start_ts, timeout):
    return start_ts + timeout - time()
