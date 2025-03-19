import shutil
import tempfile

import ujson as json
from voicetech.asr.tools.asr_analyzer.lib.run_analyzer import run_analyzer


def calculate_errors_ex(recognitions, cluster_references='cluster_references.json'):
    decoder_result_filename, references_filename = _encode_recognition_results(recognitions)

    config_filename = _write_to_temp_file(json.dumps(_get_config(cluster_references)))

    report_directory = 'report'

    run_analyzer(
        config_path=config_filename,
        decoder_result=decoder_result_filename,
        references=references_filename,
        report_directory=report_directory,
    )

    return _decode_run_analyzer_report(report_directory)


# Encode input to run_analyzer tool format
def _encode_recognition_results(recognitions):
    decoder_result = []
    references = []

    for recognition in recognitions:
        # TODO: run str.lower() conditionally based on block's input
        decoder_result.append(
            json.dumps(
                {
                    'id': recognition['id'],
                    'variants': [
                        {
                            'acoustic_score': 0,  # fake value
                            'graph_score': 0,  # fake value
                            'string': str.strip(str.lower(recognition['hyp'])),
                        }
                    ],
                }
            )
        )
        references.append('%s\t%s' % (str.strip(recognition['id']), str.strip(str.lower(recognition['ref']))))

    decoder_result = '\n'.join(decoder_result)
    references = '\n'.join(references)

    decoder_result_filename = _write_to_temp_file(decoder_result)
    references_filename = _write_to_temp_file(references)

    return decoder_result_filename, references_filename


# Decode run_analyzer tool output format
def _decode_run_analyzer_report(report_directory):
    with open('%s/IdSlice/json_report.json' % report_directory) as f:
        report = f.read()
    shutil.rmtree(report_directory)
    return report


def _write_to_temp_file(data):
    tmp_file = tempfile.NamedTemporaryFile(delete=False, encoding='utf-8', mode='w+')
    tmp_file.write(data)
    tmp_file.flush()
    tmp_file.close()
    return tmp_file.name


# TODO: move config to block's input
def _get_config(cluster_references):
    return {
        "resources": {"references": "ref", "decoder_result": "dec", "cluster_references": cluster_references},
        "statistics": {
            "sentence_level": ["WER"],
            "slice_level": [
                "CorrectWordsCount",
                "TotalWordsCount",
                "InsertedWordsCount",
                "DeletedWordsCount",
                "SubstitutedWordsCount",
                "CorrectSentencesCount",
                "SentencesWithSubstitutionsCount",
                "SentencesWithInsertionsCount",
                "SentencesWithDeletionsCount",
                "ReferenceTotalWordsCount",
                "WER",
                "TopDeletions",
                "TopSubstitutions",
                "TopInsertions",
            ],
        },
        "reports": ["JsonReport"],
        "slices": [{"kind": "id"}],
    }
