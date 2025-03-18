"""Rest api for graphs for volumes of gencfg"""

import time
import simplejson

from flask import render_template, url_for, request, jsonify

from . import app
from core.query import run_query
from core.utils import get_ch_formatted_date_from_timestamp


EMPIRICAL_SSD_VOLUMES = ['workdir_ssd_size', 'iss_resources_ssd_size', 'iss_shards_ssd_size',
                         'webcache_ssd_size', 'callisto_ssd_size', 'logs_ssd_size']


EMPIRICAL_HDD_VOLUMES = ['workdir_hdd_size', 'iss_resources_hdd_size', 'iss_shards_hdd_size',
                         'webcache_hdd_size', 'callisto_hdd_size', 'logs_hdd_size']


EMPIRICAL_VOLUMES = EMPIRICAL_SSD_VOLUMES + EMPIRICAL_HDD_VOLUMES


def __get_tss(args, min_start_ts=None):
    """Get timestamps"""
    start_ts = int(args.get('start_ts', 0))
    if min_start_ts is not None:
        start_ts = max(start_ts, min_start_ts)

    end_ts = int(args.get('end_ts', int(time.time())))
    start_ed = get_ch_formatted_date_from_timestamp(start_ts)
    end_ed = get_ch_formatted_date_from_timestamp(end_ts)

    return start_ts, end_ts, start_ed, end_ed


def __get_tss_conditions(args, min_start_ts=None):
    """Get condition for sql request"""
    start_ts, end_ts, start_ed, end_ed = __get_tss(args, min_start_ts=min_start_ts)

    return "(eventDate >= '{start_ed}') AND (eventDate <= '{end_ed}') AND (ts >= {start_ts}) AND (ts <= {end_ts})".format(
        start_ts=start_ts, end_ts=end_ts, start_ed=start_ed, end_ed=end_ed
    )


@app.route('/volumes/<group>/list')
def volumes_group_list(group):
    show_empirical = request.args.get('show_empirical') is not None
    show_real = request.args.get('show_real') is not None

    assert show_empirical is not None or show_real is not None

    return jsonify(volumes_group_list_impl(group, show_empirical=show_empirical, show_real=show_real))


def volumes_group_list_impl(group, show_empirical=True, show_real=True):
    """Return list of volumes for specified group at specified time range"""
    assert (show_empirical is True) or (show_real is True)

    # get signal volumes_info for specified group at specified time
    query = "SELECT DISTINCT volume_name FROM group_volumes_aggregated WHERE group = '{group}' AND {tss_conditions}".format(
                group=group, tss_conditions=__get_tss_conditions(request.args, min_start_ts=int(time.time() - 10 * 24 * 60 * 60)))
    data = run_query(query)

    volumes_signals = sorted(x[0] for x in data)

    result = []
    if show_empirical:
        result.extend([x for x in volumes_signals if x in EMPIRICAL_VOLUMES])
    if show_real:
        result.extend([x for x in volumes_signals if x not in EMPIRICAL_VOLUMES])

    return result


@app.route('/volumes/<group>/graph')
def volumes_group_graph(group):
    volume_name = request.args.get('volume_name')
    if volume_name is None:
        return render_template('404.html', error_msg='Missing obligatory CGI param <volume_name>')

    signal_name = '{}_{}'.format(group, volume_name).replace('/', '_')
    signal_description = 'Group {} volume {} usage (in Gb)'.format(group, volume_name)

    params_str = request.args.get('params')
    graph_base_url = url_for('volumes_group_graph_data', group=group)

    extra_cgi = 'volume_name={}'.format(volume_name)

    return render_template('qloud_graph.html', graph_base_url=graph_base_url, signal_name=signal_name, extra_cgi=extra_cgi, signal_description=signal_description)


def get_group_volume_graph_data(group_name, volume_name, show_guarantee=True, show_usage=True):
    assert (show_guarantee is True) or (show_usage is True), 'At least one of <show_guaratnee,show_usage> param must be specified'

    start_ts = int(request.args.get('start_ts', 0))
    start_ed = get_ch_formatted_date_from_timestamp(start_ts)
    end_ts = int(request.args.get('end_ts', int(time.time())))
    end_ed = get_ch_formatted_date_from_timestamp(end_ts)

    query = ("SELECT ts, volume_usage, volume_guarantee FROM group_volumes_aggregated "
             "    WHERE eventDate >= '{start_ed}' AND eventDate <= '{end_ed}' AND ts >= {start_ts} AND ts <= {end_ts} "
             "        AND group = '{group}' AND volume_name = '{volume_name}'"
             "    ORDER BY ts").format(start_ts=start_ts, start_ed=start_ed, end_ts=end_ts, end_ed=end_ed, group=group_name, volume_name=volume_name)
    query_result = run_query(query)

    if len(query_result):
        usage_data = [(int(x[0]) * 1000, float(x[1]) / 1024. / 1024. / 1024.) for x in query_result]
        guarantee_data = [(int(x[0]) * 1000, float(x[2]) / 1024. / 1024. / 1024.) for x in query_result]

        result = []
        if show_guarantee:
            result.append(dict(
                    name='Volume <b>{}</b> guarantee'.format(volume_name),
                    type='line',
                    data=guarantee_data,
            ))
        if show_usage:
            result.append(dict(
                 name='Volume <b>{}</b> usage'.format(volume_name),
                 type='line',
                 data=usage_data,
            ))
        return result
    else:
        return []


@app.route('/volumes/<group>/graph/data')
def volumes_group_graph_data(group):
    volume_name = request.args.get('volume_name')
    if volume_name is None:
        return jsonify([])

    return jsonify(get_group_volume_graph_data(group, volume_name))


@app.route('/volumes/<group>/graph/all')
def volumes_group_graph_all(group):
    show_empirical = request.args.get('show_empirical') is not None
    show_real = request.args.get('show_real') is not None

    assert show_empirical is not None or show_real is not None

    if show_empirical and show_real:
        signal_name = '{}_empirical_real'.format(group).replace('/', '_')
        signal_description = 'Group {} all volumes usage (in Gb)'.format(group)
    elif show_empirical and not show_real:
        signal_name = '{}_empirical'.format(group).replace('/', '_')
        signal_description = 'Group {} empirical volumes usage (in Gb)'.format(group)
    elif not show_empirical and show_real:
        signal_name = '{}_real'.format(group).replace('/', '_')
        signal_description = 'Group {} real volumes usage (in Gb)'.format(group)
    else:
        raise Exception('Could not happen')

    params_str = request.args.get('params')
    graph_base_url = url_for('volumes_group_graph_all_data', group=group)

    extra_cgi = []
    if show_empirical:
        extra_cgi.append('show_empirical')
    if show_real:
        extra_cgi.append('show_real')
    extra_cgi = '&'.join(extra_cgi)

    return render_template('qloud_graph.html', graph_base_url=graph_base_url, signal_name=signal_name, extra_cgi=extra_cgi, signal_description=signal_description)


@app.route('/volumes/<group>/graph/all/data')
def volumes_group_graph_all_data(group):
    show_empirical = request.args.get('show_empirical') is not None
    show_real = request.args.get('show_real') is not None

    assert show_empirical is not None or show_real is not None

    volumes = volumes_group_list_impl(group, show_empirical=show_empirical, show_real=show_real)

    result = [get_group_volume_graph_data(group, x, show_guarantee=False, show_usage=True) for x in volumes]
    result = sum(result, [])

    # filter out zerodata
    filtered_result = []
    for elem in result:
        values = list({x[1] for x in elem['data']})
        if len(values) == 1 and values[0] == 0.0:
            continue
        filtered_result.append(elem)

    return jsonify(filtered_result)

