import json
import urllib
import time

from flask import render_template, url_for

from . import app


@app.route("/test_histogram")
def test_histogram():
    ts = 1471305600

    def get_params_str(cpu_usage):
        params = {
            "ts": ts,
            "n_bins": 50,
            "graphs": [{
                "group": "SAS_WEB_NMETA_RKUB",
                "signal": "instance_cpuusage" if cpu_usage else "instance_memusage",
            }, {
                "group": "SAS_WEB_REFRESH_10DAY_BASE",
                "signal": "host_cpuusage" if cpu_usage else "host_memusage",
            }, {
                "group": ["SAS_WEB_REFRESH_3DAY_BASE",
                          "SAS_WEB_REFRESH_3DAY_BASE_COMTRBACKUP",
                          "SAS_WEB_REFRESH_10DAY_BASE",
                          "SAS_WEB_REFRESH_10DAY_BASE_COMTRBACKUP"],
                "signal": "instance_cpuusage" if cpu_usage else "instance_memusage",
            },
            ]
        }
        params_str = urllib.quote(json.dumps(params))

        return params_str

    cpu_histogram_url = url_for('cpu_histogram_data', params=get_params_str(cpu_usage=True))
    memory_histogram_url = url_for('memory_histogram_data', params=get_params_str(cpu_usage=False))

    return render_template("histogram.html", cpu_graph_url=cpu_histogram_url, memory_graph_url=memory_histogram_url)


@app.route("/test_histogram_2")
def test_histogram_2():
    ts = 1474654320

    def get_params_str(cpu_usage):
        params = {
            "ts": ts,
            "n_bins": 150,
            "graphs": [{
                "group": "SAS_WEB_TIER0_JUPITER_BASE",
                "signal": "instance_cpuusage" if cpu_usage else "instance_memusage",
            }]
        }
        params_str = urllib.quote(json.dumps(params))

        return params_str

    cpu_histogram_url = url_for('cpu_histogram_data', params=get_params_str(cpu_usage=True))
    memory_histogram_url = url_for('memory_histogram_data', params=get_params_str(cpu_usage=False))

    return render_template("histogram.html", cpu_graph_url=cpu_histogram_url, memory_graph_url=memory_histogram_url)


@app.route('/test_host_graph_2')
def test_host_graph_data_2():
    ts1 = 1471305600
    ts2 = int(time.time())
    zoom_level = "auto"

    # host = 'sas1-7241.search.yandex.net'
    host = 'ws38-058.search.yandex.net'

    params = {
        "start_ts": ts1,
        "end_ts": ts2,
        "zoom_level": zoom_level
    }
    params_str = urllib.quote(json.dumps(params))

    cpu_graph_url = url_for('host_cpu_graph_data', host=host, params=params_str)
    memory_graph_url = url_for('host_memory_graph_data', host=host, params=params_str)

    return render_template("graph.html", cpu_graph_url=cpu_graph_url, memory_graph_url=memory_graph_url)


@app.route("/test_group_graph/<group>")
@app.route("/test_group_graph", defaults={"group": "SAS_WEB_PLATINUM_BASE"})
def test_group_graph(group):
    # ts1 = 1469059200
    # ts2 = 1469318400
    ts1 = 1471305600
    ts2 = int(time.time())
    zoom_level = "auto"

    def get_params_str(cpu_usage):
        params = {
            "start_ts": ts1,
            "end_ts": ts2,
            "zoom_level": zoom_level,
            "graphs": [{
                "group": group,
                "signal": "instance_cpuusage" if cpu_usage else "instance_memusage",
                "stacking": False
            }, {
                "group": group,
                "signal": "host_cpuusage" if cpu_usage else "host_memusage",
                "stacking": False
            }, {
                "group": group,
                "signal": "instance_cpu_allocated" if cpu_usage else "instance_mem_allocated",
                "stacking": False
            }]
        }
        params_str = urllib.quote(json.dumps(params))

        return params_str

    cpu_graph_url = url_for('group_cpu_graph_data', params=get_params_str(cpu_usage=True))
    memory_graph_url = url_for('group_memory_graph_data', params=get_params_str(cpu_usage=False))

    return render_template("graph.html", cpu_graph_url=cpu_graph_url, memory_graph_url=memory_graph_url)


@app.route("/test_group_graph_2")
def test_group_graph_2():
    # ts1 = 1469059200
    # ts2 = 1469318400
    ts1 = 1471305600
    ts2 = int(time.time())
    zoom_level = "auto"

    def get_params_str(cpu_usage):
        params = {
            "start_ts": ts1,
            "end_ts": ts2,
            "zoom_level": zoom_level,
            "graphs": [{
                "group": "SAS_WEB_REFRESH_10DAY_BASE",
                "signal": "instance_cpuusage" if cpu_usage else "instance_memusage",
                "stacking": True
            }, {
                "group": "SAS_WEB_REFRESH_10DAY_BASE_COMTRBACKUP",
                "signal": "instance_cpuusage" if cpu_usage else "instance_memusage",
                "stacking": True
            }, {
                "group": "SAS_WEB_REFRESH_3DAY_BASE",
                "signal": "instance_cpuusage" if cpu_usage else "instance_memusage",
                "stacking": True
            }, {
                "group": "SAS_WEB_REFRESH_3DAY_BASE_COMTRBACKUP",
                "signal": "instance_cpuusage" if cpu_usage else "instance_memusage",
                "stacking": True
            }, {
                "group": ["SAS_WEB_REFRESH_3DAY_BASE",
                          "SAS_WEB_REFRESH_3DAY_BASE_COMTRBACKUP",
                          "SAS_WEB_REFRESH_10DAY_BASE",
                          "SAS_WEB_REFRESH_10DAY_BASE_COMTRBACKUP"],
                "signal": "instance_cpuusage" if cpu_usage else "instance_memusage",
                "stacking": False
            },
            ]
        }
        params_str = urllib.quote(json.dumps(params))

        return params_str

    cpu_graph_url = url_for('group_cpu_graph_data', params=get_params_str(cpu_usage=True))
    memory_graph_url = url_for('group_memory_graph_data', params=get_params_str(cpu_usage=False))

    return render_template("graph.html", cpu_graph_url=cpu_graph_url, memory_graph_url=memory_graph_url)


@app.route("/test_instance_graph")
def test_instance_graph():
    # ts1 = 1469059200
    # ts2 = 1469318400
    ts1 = int(time.time() - 7 * 24 * 60 * 60)
    ts2 = int(time.time())
    zoom_level = "auto"

    def get_params_str(cpu_usage):
        params = {
            "start_ts": ts1,
            "end_ts": ts2,
            "zoom_level": zoom_level,
            "graphs": [{
                "host": "man1-2233.search.yandex.net",
                "port": 20183,
                "signal": "instance_cpuusage" if cpu_usage else "instance_memusage",
            }, {
                "host": "man1-2233.search.yandex.net",
                "port": 20175,
                "signal": "instance_cpuusage" if cpu_usage else "instance_memusage",
            }, {
                "host": "man1-0155.search.yandex.net",
                "port": 20183,
                "signal": "instance_cpuusage" if cpu_usage else "instance_memusage",
            }, {
                "host": "man1-4278.search.yandex.net",
                "port": 20175,
                "signal": "instance_cpuusage" if cpu_usage else "instance_memusage",
            }]
        }
        params_str = urllib.quote(json.dumps(params))

        return params_str

    cpu_graph_url = url_for('instance_cpu_graph_data', params=get_params_str(cpu_usage=True))
    memory_graph_url = url_for('instance_memory_graph_data', params=get_params_str(cpu_usage=False))

    return render_template("graph.html", cpu_graph_url=cpu_graph_url, memory_graph_url=memory_graph_url)


