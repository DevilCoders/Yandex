# -*- coding: utf-8 -*-
import copy

from six.moves import urllib
from library.python.monitoring.solo.objects.yasm import YasmAlert, ValueModify
from library.python.monitoring.solo.helpers.yasm import signals

DEFAULT_METAGROUPS = ("ASEARCH",)


def get_yadeploy_system_alerts(prefix_name, deploy_units, stages, **kwargs):
    """
    Generates system alerts for Yandex.Deploy
    See get_system_alerts method description
    """
    tags = dict(itype=["deploy"],
                deploy_unit=deploy_units,
                stage=stages)

    return get_system_alerts(prefix_name=prefix_name, tags=tags, **kwargs)


def get_system_alerts(prefix_name, tags, mgroups=DEFAULT_METAGROUPS, **kwargs):
    """
    Generates basic system alerts calculated on portoinst metrics.
    :param prefix_name: Unique prefix for alert names, usually 'project.service.'
    :param tags: List of key-value pairs, to filter out signals for your service
    :type tags: Dict[str]
    :param mgroups - List of metagroups
    :type mgroups - List[str]
    :param kwargs - set custom parameters for separate alerts via kwargs. see example below and code
    {
        "crit": {"net_drops": [0.2, None]},
        "warn": {"net_drops": [0.1, None]},
        "juggler_check": {"default": JugglerCheck(...)}
    }
    possible alert names: [default, cpu_usage_p90, cpu_usage_max, cpu_throttled, memory_usage_p90, anon_usage, net_drops]
    :type kwargs - Dict
    :return: List of YasmAlert objects
    """
    for alert_class in [CpuUsageP90Alert, CpuUsageMaxAlert, CpuThrottledAlert,
                        MemoryUsageP90Alert, AnonUsageAlert, NetDropsAlert,
                        CoredumpsTotalAlert, OOMKillsAlert]:

        alert_name = getattr(alert_class, "_name")

        alert_kwargs = {}
        for param in kwargs:
            if alert_name in kwargs[param]:
                alert_kwargs[param] = kwargs[param][alert_name]
            elif "default" in kwargs[param]:
                alert_kwargs[param] = kwargs[param]["default"]

        check = copy.copy(alert_kwargs.pop("juggler_check", None))
        if check:
            check.service = alert_name

        alert = alert_class(
            name=prefix_name + alert_name,
            tags=tags,
            mgroups=list(mgroups),
            juggler_check=check,
            **alert_kwargs
        )

        yield alert


class YasmSystemAlert(YasmAlert):

    def __init__(self, *args, **kwargs):
        super(YasmSystemAlert, self).__init__(*args, **kwargs)

        if self.juggler_check:
            self.juggler_check.meta["urls"] += [{
                "title": "Топ по хостам",
                "type": "top_graph_url",
                "url": "{}/?top_signal={}&top_method=maxavg".format(self.get_chart_url(),
                                                                    urllib.parse.quote_plus(self.signal))
            }]


class CpuUsageP90Alert(YasmSystemAlert):
    _name = "cpu_usage_p90"

    def __init__(self, *args, **kwargs):
        kwargs["signal"] = "quant({},90)".format(signals.portoinst_cpu_limit_usage_perc_hgram)
        kwargs["warn"] = kwargs.get("warn", [70, 80])
        kwargs["crit"] = kwargs.get("crit", [80, None])
        super(CpuUsageP90Alert, self).__init__(*args, **kwargs)


class CpuUsageMaxAlert(YasmSystemAlert):
    _name = "cpu_usage_max"

    def __init__(self, *args, **kwargs):
        kwargs["signal"] = kwargs.get("signal",
                                      "quant({},max)".format(signals.portoinst_cpu_limit_usage_perc_hgram))
        kwargs["warn"] = kwargs.get("warn", [75, 85])
        kwargs["crit"] = kwargs.get("crit", [85, None])
        super(CpuUsageMaxAlert, self).__init__(*args, **kwargs)


class CpuThrottledAlert(YasmSystemAlert):
    _name = "cpu_throttled"

    def __init__(self, *args, **kwargs):
        kwargs["signal"] = kwargs.get("signal",
                                      "perc({0},div({1},{2}))".format(signals.portoinst_cpu_throttled_cores_txxx,
                                                                      signals.portoinst_cpu_limit_slot_cores_tmmv,
                                                                      signals.counter_instance_tmmv))
        kwargs["value_modify"] = kwargs.get("value_modify", ValueModify(type="aver", window=600))
        kwargs["warn"] = kwargs.get("warn", [3, 5])
        kwargs["crit"] = kwargs.get("crit", [5, None])
        super(CpuThrottledAlert, self).__init__(*args, **kwargs)


class MemoryUsageP90Alert(YasmSystemAlert):
    _name = "memory_usage_p90"

    def __init__(self, *args, **kwargs):
        kwargs["signal"] = kwargs.get("signal",
                                      "quant({},90)".format(signals.portoinst_memory_limit_usage_perc_hgram))
        kwargs["warn"] = kwargs.get("warn", [80, 90])
        kwargs["crit"] = kwargs.get("crit", [90, None])
        kwargs["value_modify"] = kwargs.get("value_modify", ValueModify(type="max", window=300))
        super(MemoryUsageP90Alert, self).__init__(*args, **kwargs)


class AnonUsageAlert(YasmSystemAlert):
    _name = "anon_usage"

    def __init__(self, *args, **kwargs):
        kwargs["signal"] = kwargs.get("signal",
                                      "or(perc({0},div({1},{2})),0)".format(signals.portoinst_anon_usage_gb_txxx,
                                                                            signals.portoinst_anon_limit_tmmv,
                                                                            signals.counter_instance_tmmv))
        kwargs["value_modify"] = kwargs.get("value_modify", ValueModify(type="aver", window=600))
        kwargs["warn"] = kwargs.get("warn", [80, 90])
        kwargs["crit"] = kwargs.get("crit", [90, None])
        super(AnonUsageAlert, self).__init__(*args, **kwargs)


class NetDropsAlert(YasmSystemAlert):
    _name = "net_drops"

    def __init__(self, *args, **kwargs):
        kwargs["signal"] = kwargs.get("signal",
                                      "perc(sum({0},{1}),sum({2},{3}))".format(signals.portoinst_net_rx_drops_summ,
                                                                               signals.portoinst_net_tx_drops_summ,
                                                                               signals.portoinst_net_rx_packets_summ,
                                                                               signals.portoinst_net_tx_packets_summ))
        kwargs["value_modify"] = kwargs.get("value_modify", ValueModify(type="aver", window=300))
        kwargs["warn"] = kwargs.get("warn", [0.001, 0.005])
        kwargs["crit"] = kwargs.get("crit", [0.005, None])
        super(NetDropsAlert, self).__init__(*args, **kwargs)


class NetUsageOverLimitAlert(YasmSystemAlert):
    _name = "net_usage_over_limit"

    def __init__(self, *args, **kwargs):
        kwargs["signal"] = kwargs.get("signal",
                                      "perc(sum({0},{1}),{2})".format(signals.portoinst_net_tx_mb_summ,
                                                                      signals.portoinst_net_rx_mb_summ,
                                                                      signals.portoinst_net_limit_mb_summ))
        kwargs["value_modify"] = kwargs.get("value_modify", ValueModify(type="aver", window=600))
        kwargs["crit"] = kwargs.get("crit", [90, None])
        super(NetUsageOverLimitAlert, self).__init__(*args, **kwargs)


class NetUsageOverGuaranteeAlert(YasmSystemAlert):
    _name = "net_usage_over_guarantee"

    def __init__(self, *args, **kwargs):
        kwargs["signal"] = kwargs.get("signal",
                                      "perc(sum({0},{1}),{2})".format(signals.portoinst_net_tx_mb_summ,
                                                                      signals.portoinst_net_rx_mb_summ,
                                                                      signals.portoinst_net_guarantee_mb_summ))
        kwargs["value_modify"] = kwargs.get("value_modify", ValueModify(type="aver", window=600))
        kwargs["crit"] = kwargs.get("crit", [90, None])
        super(NetUsageOverGuaranteeAlert, self).__init__(*args, **kwargs)


class CoredumpsTotalAlert(YasmSystemAlert):
    _name = "coredumps_total"

    def __init__(self, *args, **kwargs):
        kwargs["signal"] = kwargs.get("signal", "hsum({})".format(signals.portoinst_cores_total_hgram))
        kwargs["value_modify"] = kwargs.get("value_modify", ValueModify(type="summ", window=3600))
        kwargs["crit"] = kwargs.get("crit", [0.0000001, None])
        super(CoredumpsTotalAlert, self).__init__(*args, **kwargs)


class OOMKillsAlert(YasmSystemAlert):
    _name = "oom_kills"

    def __init__(self, *args, **kwargs):
        kwargs["signal"] = kwargs.get("signal", "hsum({})".format(signals.portoinst_ooms_slot_hgram))
        kwargs["value_modify"] = kwargs.get("value_modify", ValueModify(type="summ", window=3600))
        kwargs["crit"] = kwargs.get("crit", [0.0000001, None])
        super(OOMKillsAlert, self).__init__(*args, **kwargs)
