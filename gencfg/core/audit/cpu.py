"""Cpu audit functions"""

import time
from collections import defaultdict
import math
import re
import json

from core.db import CURDB
from gaux.aux_utils import retry_urlopen
from core.settings import SETTINGS
from core.audit.utils import get_capaview_report, have_enough_data, calculate_quantile_for_normal_group, TClickhouseGroupReport

# params for checking if new values are close to old ones
MAX_ABSOLUTE_DIFF = 5
MAX_RELATIVE_DIFF = 0.1


class TCpuSuggest(object):
    """Class with suggest info"""

    class ESuggestType(object):
        NOTHING = 'do nothing'
        NODATA = 'no data'
        LACKDATA = 'lack of data'
        MODIFYPOWER = 'modify power'
        SETPOWER = 'set power'
        CANNOTOPTIMIZE = 'can not optimize'

    __slots__ = ('group', 'suggest_type', 'msg', 'power', 'service_coeff', 'traffic_coeff')

    def __init__(self, group, suggest_type, msg, power=None, service_coeff=None, traffic_coeff=None):
        if suggest_type in (TCpuSuggest.ESuggestType.MODIFYPOWER, TCpuSuggest.ESuggestType.SETPOWER):
            if (power is None) or (service_coeff is None) or (traffic_coeff is None):
                raise Exception('Params <power> <service_coeff> <traffic_coeff> must be specified')

        self.group = group
        self.suggest_type = suggest_type
        self.msg = msg
        self.power = power
        self.service_coeff = service_coeff
        self.traffic_coeff = traffic_coeff

    @staticmethod
    def from_string(s):
        """Create suggest from string as <groupname:new_power>"""
        groupname, _, new_power = s.partition(':')
        group = CURDB.groups.get_group(groupname)
        new_power = float(new_power)
        return TCpuSuggest(group, TCpuSuggest.ESuggestType.MODIFYPOWER, 'Set instances power to <{:.3f}>'.format(new_power),
                        power=new_power, service_coeff=group.card.audit.cpu.service_coeff, traffic_coeff=group.card.audit.cpu.traffic_coeff)

    def can_apply(self):
        """Check if we have changes to apply"""
        return self.suggest_type in (TCpuSuggest.ESuggestType.MODIFYPOWER, TCpuSuggest.ESuggestType.SETPOWER)

    def apply(self, update_db=False):
        """Apply changes to group"""
        if self.suggest_type not in (TCpuSuggest.ESuggestType.MODIFYPOWER, TCpuSuggest.ESuggestType.SETPOWER):
            raise Exception('Can not apply with suggest type <{}>'.format(self.suggest_type))

        new_power = int(self.power)

        instances = self.group.get_kinda_busy_instances()
        for intlookup in (CURDB.intlookups.get_intlookup(x) for x in self.group.card.intlookups):
            intlookup.mark_as_modified()

        self.group.card.legacy.funcs.instancePower = 'exactly{}'.format(new_power)
        self.group.card.properties.cpu_guarantee_set = True
        self.group.card.audit.cpu.service_coeff = self.service_coeff
        self.group.card.audit.cpu.traffic_coeff = self.traffic_coeff

        for instance in instances:
            instance.power = new_power

        self.group.mark_as_modified()

        if update_db:
            CURDB.update(smart=True)


def suggested_close_to_current(old_power, new_power):
    """Check if new power is close to old power and we should not modify"""
    if math.fabs(old_power - new_power) < MAX_ABSOLUTE_DIFF:
        return True

    if min(old_power, new_power) / max(old_power, new_power) + MAX_RELATIVE_DIFF > 1:
        return True

    return False


def get_msg_extra(group, audit_options):
    extra = []
    if group.card.audit.cpu.service_coeff != audit_options['service_coeff']:
        extra.append('service coeff {:.2f}'.format(audit_options['service_coeff']))
    if group.card.audit.cpu.traffic_coeff != audit_options['traffic_coeff']:
        extra.append('traffic coeff {:.2f}'.format(audit_options['traffic_coeff']))
    if group.card.audit.cpu.extra_cpu != audit_options['extra_cpu']:
        extra.append('extra cpu {:.2f}'.format(audit_options['extra_cpu']))

    if len(extra):
        return ' ({})'.format(', '.join(extra))
    else:
        return ''


def analyze_for_normal_group(group, audit_options, clickhouse_result, quantile99=None):
    assert clickhouse_result is not None or quantile99 is not None

    """Analyze for normal group"""

    # set <instance_power> and <quantile> params
    if quantile99 is None:
        quantile = calculate_quantile_for_normal_group(clickhouse_result)
        instance_power = set((x.power for x in group.get_kinda_busy_instances()))
        if len(instance_power) == 1:
            instance_power = instance_power.pop()
        else:
            instance_power = None
    else:
        quantile = quantile99
        if group.card.properties.cpu_guarantee_set and group.card.audit.cpu.class_name in ('normal', 'greedy'):
            m = re.match('exactly(\d+)', group.card.legacy.funcs.instancePower)
            if m:
                instance_power = float(m.group(1))
            else:
                instance_power = None
        else:
            instance_power = None

    if group.card.properties.cpu_guarantee_set:
        suggested_power = quantile * audit_options['traffic_coeff'] * audit_options['service_coeff'] + audit_options['extra_cpu']
        suggested_power = max(audit_options['min_cpu'], suggested_power)
        if instance_power:
            if suggested_close_to_current(instance_power, suggested_power):
                msg = 'Do nothing: new power <{:.2f}> is close to old power <{:.2f}>{}'.format(instance_power, suggested_power, get_msg_extra(group, audit_options))
                return TCpuSuggest(group, TCpuSuggest.ESuggestType.NOTHING, msg, power=suggested_power, service_coeff=audit_options['service_coeff'],
                        traffic_coeff=audit_options['traffic_coeff'])
            else:
                msg = 'Modify power: <{:.2f}> => <{:.2f}> for group with instance power already set{}'.format(
                        instance_power, suggested_power, get_msg_extra(group, audit_options))
                return TCpuSuggest(group, TCpuSuggest.ESuggestType.MODIFYPOWER, msg, power=suggested_power, service_coeff=audit_options['service_coeff'],
                        traffic_coeff=audit_options['traffic_coeff'])
        else:
            msg = 'Set power to <{:.2f}> for group with instances with different instances power{}'.format(suggested_power, get_msg_extra(group, audit_options))
            return TCpuSuggest(group, TCpuSuggest.ESuggestType.SETPOWER, msg, power=suggested_power, service_coeff=audit_options['service_coeff'],
                    traffic_coeff=audit_options['traffic_coeff'])
    else:
        suggested_service_coeff = audit_options.get('service_coeff', 1.1333)
        suggested_traffic_coeff = audit_options.get('traffic_coeff', 1.5)
        suggested_power = quantile * suggested_service_coeff * suggested_traffic_coeff + audit_options.get('extra_cpu', 0)
        suggested_power = max(audit_options['min_cpu'], suggested_power)
        if instance_power:
            msg = 'Set power: <{:.2f}>  => <{:.2f}> for previously unset group{}'.format(
                instance_power, suggested_power, get_msg_extra(group, audit_options))
        else:
            msg = 'Set power to <{:.2f}> for group with instances with different instances power{}'.format(suggested_power, get_msg_extra(group, audit_options))
        return TCpuSuggest(group, TCpuSuggest.ESuggestType.SETPOWER, msg, power=suggested_power, service_coeff=suggested_service_coeff, traffic_coeff=suggested_traffic_coeff)


def suggest_for_normal_group(group, audit_options, quantile99=None):
    """Make suggest for group with audit_class=normal"""
    if group.card.properties.full_host_group:
        return TCpuSuggest(group, TCpuSuggest.ESuggestType.NOTHING, 'Full host group can not be optimized')

    if group.card.properties.fake_group:
        return TCpuSuggest(group, TCpuSuggest.ESuggestType.NOTHING, 'Fake group can not be optimized')

    if quantile99 is None and len(group.get_kinda_busy_hosts()) == 0:
        return TCpuSuggest(group, TCpuSuggest.ESuggestType.NOTHING, 'Group is empty (has no hosts)')

    clickhouse_report = None
    if quantile99 is None:
        if get_capaview_report().get('usage_stats', {}).get(TClickhouseGroupReport.EPeriods.TWO_WEEKS, {}).get(group.card.name, None) is None:
            return TCpuSuggest(group, TCpuSuggest.ESuggestType.NOTHING, 'No data for group in clickhouse')

        clickhouse_report = TClickhouseGroupReport.from_usage_dict(get_capaview_report(), group, 'instance_cpuusage_power_units')

        status, suggest = have_enough_data(group, clickhouse_report, TCpuSuggest)
        if not status:
            return suggest

    return analyze_for_normal_group(group, audit_options, clickhouse_result=clickhouse_report, quantile99=quantile99)


def suggest_for_group(group, quantile99=None, rewrite_audit_options=None):
    """Make suggestion for group

    :param group: gencfg group to work with
    :type group: core.igroups.IGroup
    :param quantile99: already calculated 99-median of usage (if not specified will be calculated)
    :type quantile99: float
    :param rewrite_audit_options: dict with overrided audit params
    :type rewrite_audit_options: dict
    """

    audit_options = group.card.audit.cpu.as_dict()
    if rewrite_audit_options:
        for k in rewrite_audit_options:
            if k not in audit_options:
                raise Exception('Unknown audit option <{}>'.format(k))
            audit_options[k] = rewrite_audit_options[k]

    audit_class = audit_options['class_name']
    if audit_class in ('normal', 'greedy'):
        return suggest_for_normal_group(group, audit_options, quantile99=quantile99)
    else:
        return TCpuSuggest(group, TCpuSuggest.ESuggestType.CANNOTOPTIMIZE, 'Group has audit type <{}> which can not be optimized right now'.format(audit_class))


