#!/usr/bin/python3

import ujson as json

import nirvana.job_context as nv

from cloud.ai.speechkit.stt.lib.data_pipeline.honeypots.check_transcript import (
    generate_honeypots,
    CheckTranscriptSolutionType,
)


def main():
    inputs = nv.context().inputs
    outputs = nv.context().outputs
    params = nv.context().parameters

    solution_type_to_count = {}

    solution_types = params.get('solution-types')
    solution_types_counts = params.get('solution-types-counts')

    assert len(solution_types) == len(solution_types_counts)
    for i in range(len(solution_types)):
        solution_type = CheckTranscriptSolutionType(solution_types[i])
        count = solution_types_counts[i]
        solution_type_to_count[solution_type] = count

    if inputs.has('levenshtein_dict.json'):
        with open(inputs.get('levenshtein_dict.json')) as f:
            levenshtein_dict = json.load(f)
    else:
        levenshtein_dict = None

    honeypots = generate_honeypots(markups_table_name_ge=params.get('markups-table-name-ge'),
                                   random_sample_fraction=params.get('random-sample-fraction'),
                                   solution_type_to_count=solution_type_to_count,
                                   solution_min_overlap=params.get('solution-min-overlap'),
                                   text_min_length=params.get('text-min-length'),
                                   text_min_words=params.get('text-min-words'),
                                   transcript_honeypots_path=inputs.get('transcript_honeypots.json'),
                                   transcript_honeypots_mutated_ratio=params.get('transcript-honeypots-mutated-ratio'),
                                   overlap_honeypots_mutated_ratio=params.get('overlap-honeypots-mutated-ratio'),
                                   lang=params.get('lang'),
                                   levenshtein_dict=levenshtein_dict)

    with open(outputs.get('honeypots.json'), 'w') as f:
        f.write(json.dumps(honeypots, ensure_ascii=False, indent=4))
