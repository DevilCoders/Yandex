import pytz
import yt.wrapper as yt
import nirvana.job_context as nv
from datetime import datetime, timezone, timedelta
from cloud.ai.speechkit.stt.lib.data_pipeline.import_data.voicetable import VoicetableData, process_chunk
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client

BUCKETS_COUNT = 50

start_period_date = datetime(year=2013, month=3, day=1, tzinfo=timezone.utc)
finish_period_date = datetime(year=2019, month=12, day=18, tzinfo=timezone.utc)
period_days = (finish_period_date - start_period_date).days

map_start_date = datetime(year=2019, month=1, day=1, tzinfo=timezone.utc)
map_finish_date = datetime(year=2020, month=6, day=1, tzinfo=timezone.utc)
map_days = (map_finish_date - map_start_date).days

step = int(map_days / BUCKETS_COUNT)

"""
Эти даты получены скриптом, который делит все данные voicetable на 50 равных бакетов.
Каждая дата за исключением последней -- дата последней записи в бакете.
Последняя дата выбрана так, чтобы превосходить все имеющиеся гарантированнно (последний бакет).
"""

dates_list = [
    '2014-03-27',
    '2014-06-26',
    '2014-09-25',
    '2014-12-24',
    '2015-12-28',
    '2017-03-09',
    '2017-04-14',
    '2017-06-09',
    '2017-08-04',
    '2017-08-18',
    '2017-09-16',
    '2017-10-15',
    '2017-11-17',
    '2017-12-14',
    '2018-01-10',
    '2018-02-15',
    '2018-04-02',
    '2018-05-01',
    '2018-05-18',
    '2018-06-09',
    '2018-06-30',
    '2018-07-17',
    '2018-08-02',
    '2018-08-24',
    '2018-09-11',
    '2018-09-25',
    '2018-10-07',
    '2018-10-17',
    '2018-10-26',
    '2018-11-09',
    '2018-11-18',
    '2018-11-30',
    '2018-12-20',
    '2019-01-14',
    '2019-02-02',
    '2019-02-24',
    '2019-03-09',
    '2019-03-17',
    '2019-04-03',
    '2019-04-22',
    '2019-05-11',
    '2019-05-29',
    '2019-06-16',
    '2019-07-03',
    '2019-07-21',
    '2019-08-08',
    '2019-09-06',
    '2019-09-30',
    '2019-10-29',
    '2030-08-01',
]


def date_to_bucket(date: datetime):
    date = f'{date.year}-{date.month:02d}-{date.day:02d}'
    res_idx = 0
    while dates_list[res_idx] < date:
        res_idx += 1

    return res_idx


def bucket_to_date(bucket: int):
    return map_start_date + timedelta(days=step * bucket)


s3 = create_client()
is_test_run = True


def import_voicetable(records_table_path: str, bucket: int):
    voicetable_data_map = [[] for _ in range(BUCKETS_COUNT)]
    for row in yt.read_table(records_table_path, format=yt.YsonFormat(encoding=None, format='text')):
        date = datetime.strptime(row[b'date'].decode('utf-8'), '%Y-%m-%d')
        bucket_id = date_to_bucket(date)
        if bucket != bucket_id:
            continue

        voicetable_data_map[bucket_id].append(
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
        if len(voicetable_data_map[bucket_id]) == 100000:
            print("Collected " + str(len(voicetable_data_map[bucket_id])) + " voicetable_data.")
            process_chunk(
                voicetable_data_map[bucket_id], received_at=bucket_to_date(bucket_id), s3=s3, is_test_run=is_test_run
            )
            voicetable_data_map[bucket_id] = []

    for id in range(BUCKETS_COUNT):
        if len(voicetable_data_map[id]) > 0:
            print("Collected " + str(len(voicetable_data_map[id])) + " voicetable_data.")
            process_chunk(voicetable_data_map[id], received_at=bucket_to_date(id), s3=s3, is_test_run=is_test_run)


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

    bucket = int(parameters.get("bucket"))

    records_table_path = parameters.get('yt_input_table')
    import_voicetable(records_table_path, bucket)


def parse_datetime_from_unix_timestamp(timestamp):
    return pytz.utc.localize(datetime.fromtimestamp(timestamp / 1000))


if __name__ == "__main__":
    main()
