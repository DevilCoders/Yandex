#!/usr/bin/python3

import ujson as json

import nirvana.job_context as nv

from cloud.ai.speechkit.stt.lib.data_pipeline.honeypots.transcript import generate_honeypots


def main():
    outputs = nv.context().outputs
    params = nv.context().parameters

    honeypots = generate_honeypots(
        markups_table_name_ge=params.get('markups-table-name-ge'),
        random_sample_fraction=params.get('random-sample-fraction'),
        honeypots_with_speech_count=params.get('honeypots-with-speech-count'),
        text_min_length=params.get('text-min-length'),
        text_min_words=params.get('text-min-words'),
        solution_min_overlap=params.get('solution-min-overlap'),
        max_unique_texts=params.get('max-unique-texts'),
        from_honeypots=params.get('from-honeypots'),
        words_blacklist=params.get('words-blacklist'),
        lang=params.get('lang'),
    )

    with open(outputs.get('honeypots.json'), 'w') as f:
        f.write(json.dumps(honeypots, ensure_ascii=False, indent=4))
