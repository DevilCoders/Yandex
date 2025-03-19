let lag = replace_nan({
    project = 'yc.ydb.ydbaas-cloud',
    cluster = '<< logbroker.get("cluster") >>',
    service = 'datastreams',
    database = '4beesefe4ji24rbv2',
    consumer = 'search-consumer-<< consumer_name_postfix >>',
    name='stream.internal_read.time_lags_milliseconds',
    cloud='yc.search',
    host='*'
}, 0);
let maxLag = top(1, 'max', lag);
let maxLagMin = round(max(maxLag) / 1000 / 60);
let topic = get_label(maxLag, 'stream');
alarm_if(maxLagMin >= 120);
