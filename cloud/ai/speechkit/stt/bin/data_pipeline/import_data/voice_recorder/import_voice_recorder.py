import os
import pytz
import json
import yt.wrapper as yt
import nirvana.job_context as nv
from datetime import datetime
from cloud.ai.speechkit.stt.lib.data_pipeline.import_data.voice_recorder import VoiceRecorderData, AcousticType, process_chunk
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client
from cloud.ai.speechkit.stt.lib.data_pipeline.import_data.records import process_file_wav
import io

s3 = create_client()


def import_voice_recorder_audios(records_table: dict, acoustic: str, dataset_name_from_table: bool, dataset_name: str,
                                 lang: str, mark: str, received_at: datetime, is_test_run: bool):
    voice_recorder_data = []

    print(f'Start import: {datetime.now()}')

    config = yt.default_config.default_config.copy()
    config['proxy']['url'] = records_table['cluster']
    config['token'] = os.environ['YT_TOKEN']
    config["read_parallel"]["enable"] = True
    config["read_parallel"]["max_thread_count"] = 12
    client = yt.YtClient(config=config)

    for row in yt.read_table(records_table['table'], format=yt.YsonFormat(encoding=None, format='text'), client=client):
        wav = row[b'wav']
        record_data = process_file_wav(io.BytesIO(wav))

        if dataset_name_from_table:
            assert b'dataset' in row, 'Column "dataset" not found'
            dataset = row[b'dataset'].decode('utf8')
            assert dataset, f'Dataset name for record {row[b"uuid"]} is empty'
        else:
            dataset = dataset_name
            assert dataset, 'Dataset name is empty'

        if row[b'mark'].decode("ascii") != "FAILED_AUDIO":
            voice_recorder_data.append(
                VoiceRecorderData(
                    row[b'assignment_id'],
                    row[b'id'],
                    row[b'mark'],
                    row[b'hypothesis'],
                    row[b'metric_name'],
                    row[b'metric_value'],
                    row[b'text'],
                    row[b'text_transformations'],
                    row[b'uuid'],
                    record_data,
                    row[b'worker_id'],
                    AcousticType(acoustic),
                    row.get(b'toloka_region') or b'Unknown',
                    dataset
                )
            )

        if len(voice_recorder_data) == 100000:
            print(f'Collected {str(len(voice_recorder_data))} voice recorder data: {datetime.now()}', flush=True)
            process_chunk(
                voice_recorder_data,
                lang=lang,
                mark=mark,
                received_at=received_at,
                s3=s3,
                is_test_run=is_test_run
            )
            voice_recorder_data = []

    if len(voice_recorder_data) > 0:
        print(f'Collected {str(len(voice_recorder_data))} voice recorder data: {datetime.now()}', flush=True)
        process_chunk(
            voice_recorder_data,
            lang=lang,
            mark=mark,
            received_at=received_at,
            s3=s3,
            is_test_run=is_test_run
        )


def main():
    job_context = nv.context()
    parameters = job_context.get_parameters()
    inputs = job_context.get_inputs()

    is_test_run = parameters.get("test_run")
    use_dataset_from_table = parameters.get("use_dataset_from_table")
    dataset_name = parameters.get("dataset_name")
    received_at = parameters.get("received_at")
    acoustic = parameters.get("acoustic")
    lang = parameters.get("lang")
    mark = parameters.get("mark")
    received_at = pytz.utc.localize(datetime.strptime(received_at, '%Y-%m-%dT%H:%M:%S+0300'))

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

    table_path = inputs.get('table')
    with open(table_path) as table_json_file:
        records_table = json.load(table_json_file)

    import_voice_recorder_audios(records_table, acoustic, use_dataset_from_table, dataset_name,
                                 lang, mark, received_at, is_test_run)
