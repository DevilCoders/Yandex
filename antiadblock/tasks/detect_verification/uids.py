import yt.wrapper as yt

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.yt_utils import ensure_yt_dynamic_table
from antiadblock.tasks.detect_verification.config import ADB_EXTS_PLACEHOLDER


logger = create_logger('detect_verification_logger')


SCHEMA = [
    {"name": "uid", "type": "string", "sort_order": "ascending"},
    {"name": "adb", "type": "boolean"},
    {"name": "exts", "type": "any"},
    {"name": "updated", "type": "string"},
]

YQL_QUERY = '''-- antiadblock/tasks/detect_verification/uids.py
$barnavig_logs_path = "logs/bar-navig-log/1d";
$uids_state_path = "__UID_TABLE__";
$script = @@
from json import loads
ADBLOCK_EXTS = (__ADB_EXTS__)

def extract_extentions(decoded_bundle):
    adb_exts = []
    extensions = filter(lambda _dict: _dict.get('client_id')=='instextens', loads(decoded_bundle))
    for exts in extensions:
        extension_list = exts.get('params', {}).get('extension', [])
        for extension in extension_list:
            if len(extension) <= 4:
                continue
            extid, ver, name, off, source = extension[:5]
            if off or extid not in ADBLOCK_EXTS:
                continue
            adb_exts.append(extid)
    return adb_exts
@@;
$ext_extractor = Python::extract_extentions(Callable<(String?)->List<String>>, $script);

$uid_exts = (
    SELECT uid, ListUniq(ListFlatMap(AGGREGATE_LIST(exts, 100), ($x) -> {RETURN $x;})) as exts
    from (
        SELECT yandexuid as uid, $ext_extractor(decoded_bundle) as exts
        FROM RANGE($barnavig_logs_path, "__BARNAVIG_START__", "__BARNAVIG_END__")
        where yasoft="yabrowser" and decoded_bundle is not null
    )
    group by uid
);

SELECT new.uid as uid, new.adb as adb, new.exts as exts
from
    (SELECT uid, ListAggregate(exts, AGGREGATION_FACTORY("count")) > 0 as adb, exts from $uid_exts
     where LENGTH(uid) >= 11 and LENGTH(uid) <= 64) as new
    left join $uids_state_path as old on new.uid = old.uid
where
    old.uid is null or  -- completely new uids
    new.adb != old.adb or  -- or uids with changed state
    ListSortAsc(new.exts) != ListSortAsc(ListMap(Yson::ConvertToList(old.exts), ($item) -> {return Yson::ConvertToString($item);}))
'''.replace('__ADB_EXTS__', ADB_EXTS_PLACEHOLDER)


def update_uids_state(yql_client, yt_client, table_path, start, end):
    ensure_yt_dynamic_table(yt_client, table_path, SCHEMA)
    logger.info('Getting new uids and uids with changed state from bar-navig-log')
    query = 'PRAGMA yt.Pool="antiadb";\n' + YQL_QUERY
    yql_request = yql_client.query(query
                                   .replace('__UID_TABLE__', table_path.lstrip('/'))
                                   .replace('__BARNAVIG_START__', start.strftime('%Y-%m-%d'))
                                   .replace('__BARNAVIG_END__', end.strftime('%Y-%m-%d')), syntax_version=1)
    yql_response = yql_request.run()
    logger.info('Preparing json with new uids and uids with changed state')
    data = []
    for table in yql_response.get_results():
        table.fetch_full_data()
        for row in table.rows:
            data.append({'uid': row[0], 'adb': bool(row[1]), 'exts': row[2], 'updated': end.strftime('%Y-%m-%d')})
    logger.info('Uploading new uids and uids with changed state to PATH={}'.format(table_path))
    logger.info('Uploading ({} rows) data:'.format(len(data)))
    chunk_size = 1000
    for i in range(0, len(data), chunk_size):
        yt_client.insert_rows(table_path, data[i: i + chunk_size], raw=False, format=yt.JsonFormat(attributes={"encode_utf8": False}))
    logger.info('Data uploaded')
