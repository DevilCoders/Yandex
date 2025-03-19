import typing

from cloud.ai.lib.python.datetime import parse_datetime
from cloud.ai.speechkit.stt.lib.data.model.common.hash import uint_to_bytes
from cloud.ai.speechkit.stt.lib.data.model.dao import S3Object, RecordRequestParams, HashVersion
from .record_creator import RecordData
from ..queries import run_yql_select_query


def select_records_data_from_proxy_logs(table_name) -> typing.List[RecordData]:
    yql_query = f"""
SELECT
    `time`,
    `fields`
FROM hahn.`home/logfeller/logs/yc-ai-prod-logs-services-proxy/1d/{table_name}`
WHERE
    `msg` LIKE 'STT audio is uploaded to S3%' AND
    (
        Yson::Contains(Yson::Lookup(Yson::Lookup(`fields`, "response"), "meta"), "status") AND
        Yson::LookupString(Yson::Lookup(Yson::Lookup(Yson::Lookup(`fields`, "response"), "meta"), "status"), "code") = 'OK'
    )
"""

    records_data = []

    tables, _ = run_yql_select_query(yql_query)
    rows = tables[0]
    for row in rows:
        time = row['time']
        fields = row['fields']
        request = fields['request']
        audio = request['data']['audio']
        s3_obj = audio['s3Object']
        records_data.append(
            RecordData(
                id=audio['id'],
                s3_obj=S3Object(endpoint=s3_obj['endpoint'], bucket=s3_obj['bucket'], key=s3_obj['key']),
                hash=uint_to_bytes(audio['crc64iso'], length=8),
                hash_version=HashVersion.CRC_64_ISO,
                req_params=RecordRequestParams(recognition_spec=request['data']['recognitionSpec']),
                duration_seconds=request['stats']['recordDurationSec'],
                audio_channel_count=request['stats'].get('audioChannelCount'),
                size_bytes=request['stats']['receivedBytesCount'],
                folder_id=request['meta']['folderID'],
                method=request['meta']['product'],
                received_at=parse_datetime(time),
            )
        )

    return records_data
