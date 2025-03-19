import pytz
from datetime import datetime

import yt.wrapper as yt
import nirvana.job_context as nv

from cloud.ai.lib.python.datetime import now
from cloud.ai.speechkit.stt.lib.data_pipeline.import_data.voicetable import VoicetableData, process_chunk
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client

received_at = now()
s3 = create_client()
is_test_run = True


def import_voicetable(records_table_path: str):
    voicetable_data = []
    for row in yt.read_table(records_table_path, format=yt.YsonFormat(encoding=None, format='text')):

        voicetable_data.append(
            VoicetableData(
                row[b'other_info'],
                row[b'original_info'],
                row[b'language'],
                row[b'text_orig'],
                row[b'text_ru'],
                row[b'acoustic'],
                row[b'application'],
                row[b'dataset_id'],
                row[b'date'],
                row[b'speaker_info'],
                row[b'wav'],
                row[b'duration'],
                row[b'uttid'],
                row[b'mark'],
            )
        )
        if len(voicetable_data) == 100000:
            print("Collected " + str(len(voicetable_data)) + " voicetable_data.")
            process_chunk(voicetable_data, received_at=received_at, s3=s3, is_test_run=is_test_run)
            voicetable_data = []

    if len(voicetable_data) > 0:
        print("Collected " + str(len(voicetable_data)) + " voicetable_data.")
        process_chunk(voicetable_data, received_at=received_at, s3=s3, is_test_run=is_test_run)


def main():
    job_context = nv.context()
    parameters = job_context.get_parameters()

    global is_test_run
    is_test_run = parameters.get("test_run")

    yt.config["read_parallel"]["enable"] = True
    yt.config["read_parallel"]["max_thread_count"] = 8
    yt.config["write_parallel"]["enable"] = True
    yt.config["write_parallel"]["max_thread_count"] = 8

    if is_test_run:
        print('Creating YT tables in Hume')
        yt.config["proxy"]["url"] = "hume"
    else:
        print('Creating YT tables in Hahn')
        yt.config["proxy"]["url"] = "hahn"

    records_table_path = parameters.get('yt_input_table')
    import_voicetable(records_table_path)


def parse_datetime_from_unix_timestamp(timestamp):
    return pytz.utc.localize(datetime.fromtimestamp(timestamp / 1000))


if __name__ == "__main__":
    main()
