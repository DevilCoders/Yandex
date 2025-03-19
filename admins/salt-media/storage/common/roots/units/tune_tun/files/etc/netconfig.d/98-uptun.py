import re

def process(interfaces, params):
    for i in interfaces:
        if i.name == "ip6tun0":
            new_preup = [re.sub(r"remote .* local", "remote 2a02:6b8:0:3400::aaaa local", s) if ' remote ' in s else None for s in i.preup]
            i.preup = new_preup

def filter(params):
  return True

