#!/usr/bin/env python

import re

def process(interfaces, params):
    for i in interfaces:
        if i.name == "4to6tun0":
            new_preup = [re.sub(r"2a02:6b8:0:3400::aaaa", "2a02:6b8:0:3400::cccc", s) if ' remote ' in s else s for s in i.preup]
            i.preup = new_preup

def filter(params):
  return True

