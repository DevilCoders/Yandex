import argparse
import os
import traceback

from utils import audio_utils
from utils import transcribation_utils
from utils import s3_utils

from joblib import Parallel, delayed
import json


"""
For now only requests through HTTP endpoint on prod cluster are implemented.

Note that two keys are needed:
    1. Access Key for s3 storage. See https://cloud.yandex.ru/docs/storage/instruments/aws-cli
    2. Api Key for STT long audio recognition. See https://cloud.yandex.ru/docs/speechkit/stt/transcribation#pered-nachalom
"""


DEFAULT_BUCKET_NAME = "transcribation-audio"


def save_txt(file, transcribation, text):
    file_basepath, _ = os.path.splitext(file)

    # For now not logging raw response, just text

    txt_path = file_basepath + ".txt"
    with open(txt_path, "w", encoding="utf8") as out:
        out.write(text)


def process_file(file, args):
    transcribation = None
    text = ""

    if not args.no_stdout:
        print("processing " + file)

    try:
        converted_file, finfo = audio_utils.convert_for_transcribation(file)
        uri = s3_utils.upload_file(converted_file, args.bucket)

        # File is uploaded to s3, no need to keep it locally
        if not args.no_cleanup:
            os.remove(converted_file)

        transcribation = transcribation_utils.transcribe(uri, converted_file, finfo, args)
        text = transcribation_utils.text_from_response(transcribation, file)

        if not args.no_stdout:
            print("""{{
    "file": "{}",
    "text": "{}"
}}""".format(file, text))

        if not args.no_txt:
            save_txt(file, transcribation, text)
    except Exception as e:
        print("Failed to process " + file, e, traceback.format_exc())


def main(args):
    # Dont create additional workers if there's only a single file
    njobs = 1
    if os.path.isdir(args.path):
        njobs = args.n_workers

    Parallel(n_jobs=njobs, backend="threading")(
        delayed(process_file)(file, args) for file in audio_utils.list_audio_files(args.path))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Utils for working with transcribation API.")
    parser.add_argument("path", help="Path to either a directory with audio files or a single audio file.")
    parser.add_argument("api_key", metavar="api-key", help="Api-Key for YC authorization.")
    parser.add_argument("--no-txt", dest="no_txt", default=False,
            help="Indicates that results should not be saved to '.txt' file. Default is False.")
    parser.add_argument("--no-stdout", dest="no_stdout", default=False,
            help="Indicates that results should not be printed in stdout. Default is False.")
    parser.add_argument("--lang", dest="lang", default="ru-RU",
            help="Language of audio files. Default is 'ru-RU'.")
    parser.add_argument("--raw-results", dest="raw_results", default=True,
            help="'raw_results' flag in SpeechKit. Default is True.")
    parser.add_argument("--bucket", dest="bucket", default=DEFAULT_BUCKET_NAME,
            help="Bucket name of the bucket to upload files to. Default is '%s'." % DEFAULT_BUCKET_NAME)
    parser.add_argument("--n-workers", dest="n_workers", default=10,
            help="Number of workers to use for parallel processing. Default is 10.")
    parser.add_argument("--no-cleanup", dest="no_cleanup", default=False,
            help="Don't delete newly created converted files after transcribation is finished. Default is False")

    args = parser.parse_args()

    main(args)
