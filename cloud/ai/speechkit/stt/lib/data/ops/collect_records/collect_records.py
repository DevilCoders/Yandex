from datetime import datetime, timedelta

from cloud.ai.lib.python.datasource.yt.ops import Table

from ..yt import table_records_meta, table_records_audio_meta, table_records_tags_meta
from .proxy_logs import select_records_data_from_proxy_logs
from .record_creator import create_record_from_data


def collect_records(date: datetime):
    """
    Collects STT audio records from proxy logs and creates daily table with this records.

    We query daily YT tables for logs in Hahn cluster. New daily logs tables in Hahn are created
    by Logfeller at 00:00 MSK, so we create new tables with collected records also in this timezone.

    Logs can be appended to daily table up to three days, so in this operation we query "four days before" logs table.
    """
    collect_date = date - timedelta(days=4)

    table_name = Table.get_name(collect_date)

    table_records = Table(meta=table_records_meta, name=table_name)
    table_records_audio = Table(meta=table_records_audio_meta, name=table_name)
    table_records_tags = Table(meta=table_records_tags_meta, name=table_name)

    records = []
    records_audio = []
    records_tags = []

    for record_data in select_records_data_from_proxy_logs(table_name):
        record, record_audio, record_tags = create_record_from_data(record_data)
        records.append(record)
        records_audio.append(record_audio)
        for record_tag in record_tags:
            records_tags.append(record_tag)

    table_records.append_objects(records)
    table_records_audio.append_objects(records_audio)
    table_records_tags.append_objects(records_tags)
