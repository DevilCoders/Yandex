#!/usr/bin/env python

import argparse
import json
import random


parser = argparse.ArgumentParser()
# parser.add_argument('-u', '--user-guid', action='append', type=str, required=True, help="user guid")
parser.add_argument('-i', '--input', action='append', type=str, required=True, help="path to telegram chat history")
parser.add_argument('-l', '--learn', type=str, required=True, help="path to learn")
parser.add_argument('-t', '--test', type=str, required=True, help="path to test")
parser.add_argument('-p', '--learn-percentage', type=int, default=70, help="percentage of records that will go to learn")

args = parser.parse_args()

learn = open(args.learn, "w")
test = open(args.test, "w")

for path in args.input:
    with open(path) as f:
        history = json.load(f)

    messages = dict()
    for m in history["messages"]:
        text = m["text"]
        if not len(text) or isinstance(text, list):
            continue

        messages[m["id"]] = m["text"]
        reply_to = m.get("reply_to_message_id")
        if reply_to is not None:
            context = messages.get(reply_to)
            if context is not None:
                context = context.replace("\n", ". ")
                text = text.replace("\n", ". ")
                record = ("%s\t%s" % (context, text)).encode("utf-8")
                if random.randrange(0, 100) < args.learn_percentage:
                    learn.write(record)
                    learn.write("\n")
                else:
                    test.write(record)
                    test.write("\n")
