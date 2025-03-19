import logging

import nirvana.job_context as nv

from check_data import check_tsv_zip, check_text_audio_match
from upload_data import upload_data

logging.getLogger('numba').setLevel(logging.WARNING)
logging.getLogger("yt.packages.requests").setLevel(logging.WARNING)
logging.getLogger("yt.packages.urllib3").setLevel(logging.WARNING)


def main():
    job_context = nv.context()
    inputs = job_context.get_inputs()
    outputs = job_context.get_outputs()
    parameters = job_context.get_parameters()

    audio_archive_path = inputs.get('audio')
    texts_tsv_path = inputs.get('texts')
    language = parameters.get('language')
    min_sample_rate = parameters.get('min-sample-rate')
    levenshtein_threshold = parameters.get('metric-threshold')
    stt_api_key = parameters.get('api-key')
    output_table_path = parameters.get('table-name')

    good_samples, bad_samples = check_tsv_zip(texts_tsv_path, audio_archive_path, language, min_sample_rate)
    if language in {"he"}:
        asr_bad_samples = []

    else:
        good_samples, asr_bad_samples = check_text_audio_match(good_samples,
                                                               language,
                                                               levenshtein_threshold,
                                                               stt_api_key)
    upload_data(good_samples + bad_samples + asr_bad_samples, output_table_path)

    with open(outputs.get('output_table'), 'w') as f:
        f.write(f'{{"cluster": "hahn", "table": "{output_table_path}"}}')


if __name__ == '__main__':
    main()
