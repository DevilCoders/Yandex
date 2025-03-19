#!/usr/bin/env python
from __future__ import print_function
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("old", nargs=1)
parser.add_argument("new", nargs=1)
args = parser.parse_args()

text2tts = dict()
with open(args.old[0]) as fin:
    for line in fin:
        text, tts = line.strip('\n').split('\t')
        text2tts[text.decode("utf8")] = tts.decode("utf8")

with open(args.new[0]) as fin:
    for line in fin:
        text, tts = line.strip('\n').split('\t')
        text, tts = text.decode("utf8"), tts.decode("utf8")
        assert text in text2tts
        if tts:
            if not text2tts[text]:
                print("NEW_TTS", text.encode("utf8"), tts.encode("utf8"), sep="\t")
            elif text2tts[text] != tts:
                print("CHANGED_TTS", text.encode("utf8"), text2tts[text].encode("utf8"), tts.encode("utf8"), sep="\t")
        elif text2tts[text]:
            print("MISSING_TTS", text.encode("utf8"), text2tts[text].encode("utf8"), sep="\t")
