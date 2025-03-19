#!/usr/bin/python3

from datetime import datetime

from cloud.ai.speechkit.stt.lib.data.ops.collect_records import collect_records


def main():
    collect_records(datetime.utcnow())
