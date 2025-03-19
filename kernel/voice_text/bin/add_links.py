#!/usr/bin/env python
import sys
import urllib

for line in sys.stdin:
    text = line.strip()
    print "{}\thttp://tts.voicetech.yandex.net/tts?".format(text) + urllib.urlencode({"text": text})
