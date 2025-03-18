from flask import jsonify, request, render_template, make_response
import time
from datetime import datetime
from collections import OrderedDict

from core.utils import get_ch_formatted_date_from_timestamp
from core.query import EBase, get_base_for_table, run_single_int_query, run_query, get_roughly_last_event_date_from_table
from . import app


def _get_last_recent_timestamp(table_name):
    last_event_date = get_roughly_last_event_date_from_table(table_name)
    if last_event_date:
        query = "SELECT max(ts) FROM {table_name} WHERE eventDate >= '{ed}'".format(
            table_name=table_name,
            ed=last_event_date,
        )
        return run_single_int_query(query, base_type=get_base_for_table(table_name))
    return None


@app.route("/last_recent_timestamp/<zoom_level>")
@app.route("/last_recent_timestamp", defaults={"zoom_level": "2m"})
def last_timestamp(zoom_level):
    return jsonify(result=_get_last_recent_timestamp("instanceusage_aggregated_{}".format(zoom_level)))


def get_tables_last_update():
    """Generate last_update info for tables"""
    DELAY_REALTIME = 60
    DELAY_2M = 40 * 60
    DELAY_15M = 2 * 60 * 60
    DELAY_1H = 6 * 60  * 60
    DELAY_1D = 60 * 60 * 60


    TABLES_DATA = [
        # instanceusage
        ('instanceusage', DELAY_REALTIME),
        ('instanceusage_aggregated_2m', DELAY_2M),
        ('instanceusage_aggregated_15m', DELAY_15M),
        ('instanceusage_aggregated_1h', DELAY_1H),
        ('instanceusage_aggregated_1d', DELAY_1D),
        # hostusage
        ('hostusage', DELAY_REALTIME),
        ('hostusage_aggregated_2m', DELAY_2M),
        ('hostusage_aggregated_15m', DELAY_15M),
        ('hostusage_aggregated_1h', DELAY_1H),
        ('hostusage_aggregated_1d', DELAY_1D),
        # qloudusage
        ('qloudusage', DELAY_REALTIME),
        ('qloudusage_aggregated_2m', DELAY_2M),
        ('qloudusage_aggregated_15m', DELAY_15M),
        ('qloudusage_aggregated_1h', DELAY_1H),
        ('qloudusage_aggregated_1d', DELAY_1D),
        # openstackusage
        ('openstackusage', DELAY_REALTIME),
        ('openstackusage_aggregated_2m', DELAY_2M),
        ('openstackusage_aggregated_15m', DELAY_15M),
        ('openstackusage_aggregated_1h', DELAY_1H),
        ('openstackusage_aggregated_1d', DELAY_1D),
        # abcusage
        ('abcusage', DELAY_2M),
        ('abcusage_aggregated_2m', DELAY_2M),
        ('abcusage_aggregated_15m', DELAY_15M),
        ('abcusage_aggregated_1h', DELAY_1H),
        ('abcusage_aggregated_1d', DELAY_1D),
        # ypusage
        ('ypusage', DELAY_2M),
        ('ypusage_aggregated_2m', DELAY_2M),
        ('ypusage_aggregated_15m', DELAY_15M),
        ('ypusage_aggregated_1h', DELAY_1H),
        ('ypusage_aggregated_1d', DELAY_1D),
        # other
        ('instanceusage_infreq_v2', DELAY_15M),
        ('group_allocated_resources', DELAY_1D),
    ]

    result = []
    for table_name, allow_delay in TABLES_DATA:
        try:
            last_modified = _get_last_recent_timestamp(table_name)
        except Exception:
            last_modified = 0
        last_modified_str = datetime.fromtimestamp(last_modified).strftime('%Y-%m-%d %H:%M:%S')

        # we do not want last added entry a long ago as well as in future
        if (time.time() - last_modified > allow_delay):
            in_danger = True
        else:
            in_danger = False

        result.append((table_name, last_modified_str, in_danger))

    return result


def get_2m_data_percents():
    """Check how much data from 15m table missing in 2m table (RX-576)"""
    HOUR = 60 * 60

    TABLES_DATA = [
        ('instanceusage', 'port != 65535', HOUR * 3, HOUR),
        ('qloudusage', None, HOUR * 3, HOUR),
        ('abcusage', None, HOUR * 3, HOUR),
    ]

    result = []
    for table_name, extra_condition, start_shift, end_shift in TABLES_DATA:
        now = int(time.time())
        start_ts = now - start_shift
        start_ed = get_ch_formatted_date_from_timestamp(start_ts)
        end_ts = now - end_shift
        end_ed = get_ch_formatted_date_from_timestamp(end_ts)

        if extra_condition is not None:
            extra_condition = ' AND {}'.format(extra_condition)
        else:
            extra_condition = ''

        query_tpl = ("SELECT COUNT(*) FROM {table_name}_aggregated_{{time_postfix}}"
                    "    WHERE eventDate >= '{start_ed}' AND eventDate <= '{end_ed}' AND ts >= {start_ts} AND ts <= {end_ts} {extra_condition}"
                    "    GROUP BY ts;").format(table_name=table_name, start_ed=start_ed, start_ts=start_ts, end_ed=end_ed, end_ts=end_ts,
                                               extra_condition=extra_condition)

        query_2m = query_tpl.format(time_postfix='2m')

        data_2m = run_query(query_2m)
        avg_instances_2m = sum(int(x[0]) for x in data_2m) / max(len(data_2m), 1)
        if not avg_instances_2m:
            avg_instances_2m = 1

        query_15m = query_tpl.format(time_postfix='15m')
        data_15m = run_query(query_15m)
        avg_instances_15m = sum(int(x[0]) for x in data_15m) / max(len(data_15m), 1)
        if not avg_instances_15m:
            avg_instances_15m = 1

        have_data_perc = '{:.2f}%'.format(avg_instances_2m / float(avg_instances_15m) * 100.)

        result.append(('{} 2m ratio'.format(table_name), have_data_perc, avg_instances_2m / float(avg_instances_15m) < 0.98))

    return result


@app.route('/')
def index():
    zoom_level_to_last_date = get_tables_last_update()

    # ================================ RX-576 START ==================================
    have_2m_data_percents = get_2m_data_percents()
    # ================================ RX-576 STOP ===================================

    return render_template("index.html", zoom_level_to_last_date=zoom_level_to_last_date+have_2m_data_percents)


@app.route('/hosts_by_date_graph')
def hosts_by_date_graph():
    def _run_and_parse_date_count_query(query):
        return [(datetime(*map(int, event_date.split('-'))), int(count)) for (event_date, count) in run_query(query)]

    hosts_query = """
        SELECT eventDate, count() FROM (
            SELECT DISTINCT eventDate, host FROM hostusage_aggregated_1d WHERE eventDate >= '2017-03-01'
        ) GROUP BY eventDate ORDER BY eventDate
    """
    data_hosts = _run_and_parse_date_count_query(hosts_query)

    hosts_with_instances_query = """
        SELECT eventDate, count() FROM (
            SELECT DISTINCT eventDate, host FROM instanceusage_aggregated_1d WHERE eventDate >= '2017-03-01' AND port <> 65535
        ) GROUP BY eventDate ORDER BY eventDate
    """
    data_hosts_with_instances = _run_and_parse_date_count_query(hosts_with_instances_query)

    instances_query = """
        SELECT eventDate, count() FROM (
            SELECT DISTINCT eventDate, host, port FROM instanceusage_aggregated_1d WHERE eventDate >= '2017-03-01' AND port <> 65535
        ) GROUP BY eventDate ORDER BY eventDate
    """
    data_instances = _run_and_parse_date_count_query(instances_query)

    return jsonify([{
        'name': 'Hosts count',
        'type': 'line',
        'data': data_hosts
    }, {
        'name': 'Hosts with instances count',
        'type': 'line',
        'data': data_hosts_with_instances
    }, {
        'name': 'Instances count',
        'type': 'line',
        'data': data_instances
    }])


@app.route('/upload_rate_graph')
def upload_rate_graph():
    tables = request.args.get('tables', 'instanceusage')
    tables = tables.split(',')

    ts2 = int(time.time())
    ts2 -= ts2 % 60
    ts1 = ts2 - 6 * 60 * 60

    query_records_tpl = ("SELECT ts - ts % {{aggregate_interval}} as ts_rounded, count() "
                         "FROM {{table}} "
                         "WHERE eventDate >= '{ed1}' AND eventDate <= '{ed2}' AND ts >= {ts1} AND ts <= {ts2} "
                         "GROUP BY ts_rounded "
                         "ORDER BY ts_rounded;").format(ts1=ts1, ts2=ts2, ed1=get_ch_formatted_date_from_timestamp(ts1), ed2=get_ch_formatted_date_from_timestamp(ts2))
    query_hosts_tpl = ("SELECT ts_rounded, count() FROM "
                       "(SELECT DISTINCT host, (ts - ts % {{aggregate_interval}}) as ts_rounded from {{table}} "
                       " WHERE eventDate >= '{ed1}' AND eventDate <= '{ed2}' AND ts >= {ts1} AND ts <= {ts2}) "
                       "GROUP BY ts_rounded "
                       "ORDER BY ts_rounded;").format(ts1=ts1, ts2=ts2, ed1=get_ch_formatted_date_from_timestamp(ts1), ed2=get_ch_formatted_date_from_timestamp(ts2))


    result = []
    for table_name in tables:
        instances_query = query_records_tpl.format(table=table_name, aggregate_interval=60)
        instances_records = [(int(ts) * 1000, int(count)) for (ts, count) in run_query(instances_query, base_type=get_base_for_table(table_name), raise_failed=False)]
        result.append(dict(name='Unique records'.format(table_name), type='line', data=instances_records))

        hosts_query = query_hosts_tpl.format(table=table_name, aggregate_interval=60)
        hosts_records = [(int(ts) * 1000, int(count)) for (ts, count) in run_query(hosts_query, base_type=get_base_for_table(table_name), raise_failed=False)]
        result.append(dict(name='Unique hosts'.format(table_name), type='line', data=hosts_records))

    return jsonify(result)


@app.route('/sandbox_schedulers_health_status')
def sandbox_schedulers_health_status():
    """Check for health status of sandbox schedulers (GENCFG-1872)"""

    HOUR = 60 * 60
    DAY = 24 * HOUR

    SCHEDULERS_MAX_LAG = dict(
        sandbox_scheduler_9963=DAY,
        sandbox_scheduler_9499=DAY,
        sandbox_scheduler_9498=DAY,
        sandbox_scheduler_8779=DAY,
        # sandbox_scheduler_8573=DAY*2,
        sandbox_scheduler_8295=DAY*2,
        sandbox_scheduler_7991=HOUR*3,
        sandbox_scheduler_7335=DAY*2,
        sandbox_scheduler_6104=HOUR*3,
        sandbox_scheduler_6096=DAY*2,
        sandbox_scheduler_4227=DAY*2,
        sandbox_scheduler_2504=HOUR*2,
        sandbox_scheduler_2489=DAY*2,
        sandbox_scheduler_2074=DAY*2,
        sandbox_scheduler_1919=DAY*2,
        sandbox_scheduler_1512=DAY*2,
        sandbox_scheduler_1307=HOUR*2,
        sandbox_scheduler_981=DAY*2,
        sandbox_scheduler_932=DAY*2,
        sandbox_scheduler_931=DAY*2,
        sandbox_scheduler_927=DAY*2,
        sandbox_scheduler_926=HOUR*12,
        sandbox_scheduler_924=DAY*2,
    )

    result = []
    for scheduler_name, scheduler_max_lag in SCHEDULERS_MAX_LAG.iteritems():
        query = ("SELECT value, timestamp FROM kpi_graphs_data WHERE"
                 "    timestamp = (SELECT max(timestamp) FROM kpi_graphs_data WHERE signal_name == '{scheduler_name}') AND"
                 "    signal_name == '{scheduler_name}';").format(scheduler_name=scheduler_name)

        query_result = run_query(query)

        if len(query_result) == 0:
            status = 'failure'
            msg = 'No data for scheduler {scheduler_name}'.format(scheduler_name=scheduler_name)
        else:
            value, timestamp = query_result[0]
            value = float(value)
            timestamp = float(timestamp)

            lag = value + time.time() - timestamp

            if lag > scheduler_max_lag:
                status = 'failure'
                msg = 'Last success for {} was {:.2f} seconds ago ({:.2f} allowed)'.format(scheduler_name, lag, scheduler_max_lag)
            else:
                status = 'success'
                msg = None

        result.append(dict(scheduler=scheduler_name, status=status, msg=msg))

    if set(x['status'] for x in result) != set(['success']):
        result = [x for x in result if x['status'] != 'success']
        return make_response(jsonify(dict(status='failure', schedulers=result)), 503)
    else:
        return make_response(jsonify(dict(status='success', schedulers=result)), 200)


@app.route('/clickhouse_data_upload_status')
def clickhouse_data_upload_status():
    failed_tables_info = []
    for table_name, last_modified_str, in_danger in get_tables_last_update():
        if in_danger:
            failed_tables_info.append(dict(table=table_name, msg='No points since <{}>'.format(last_modified_str)))

    if len(failed_tables_info):
        return make_response(jsonify(dict(status='failure', failed_tables=failed_tables_info)), 503)
    else:
        return make_response(jsonify(dict(status='success', failed_tables=[])), 200)


@app.route('/aggregated_2m_data_percents')
def aggregated_2m_data_percents():
    """RX-576"""
    failed_tables_info = []
    for table_name, have_data_perc, in_danger in get_2m_data_percents():
        if in_danger:
            failed_tables_info.append(dict(table=table_name, msg='Have only {} of data in 2m graph'.format(have_data_perc)))

    if len(failed_tables_info):
        return make_response(jsonify(dict(status='failure', failed_tables=failed_tables_info)), 503)
    else:
        return make_response(jsonify(dict(status='success', failed_tables=[])), 200)
