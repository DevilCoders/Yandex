#!/usr/bin/python3

import ujson as json

import nirvana.job_context as nv

from cloud.ai.speechkit.stt.lib.experiments.processing import select_records


def main():
    outputs = nv.context().outputs
    params = nv.context().parameters

    records = select_records(params.get('tags'), params.get('marks'))

    with open(outputs.get('records'), 'w') as f:
        f.write(json.dumps(records, ensure_ascii=False, indent=4))
