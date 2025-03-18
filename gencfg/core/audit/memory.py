"""Memory audit functions"""

import math

import gencfg
from core.card.types import ByteSize

from core.audit.utils import get_capaview_report, have_enough_data, calculate_quantile_for_normal_group, TClickhouseGroupReport


# params for checking if new values are close to old ones
MAX_ABSOLUTE_DIFF = 0.2
MAX_RELATIVE_DIFF = 0.03



class TMemorySuggest(object):
    """Class with suggest info"""

    class ESuggestType(object):
        NOTHING = 'do nothing'
        NODATA = 'no data'
        LACKDATA = 'lack of data'
        MODIFY_GUARANTEE = 'modify memory guarantee'

    __slots__ = ('group', 'suggest_type', 'msg', 'guarantee')

    def __init__(self, group, suggest_type, msg, guarantee=None):
        if suggest_type in (TMemorySuggest.ESuggestType.MODIFY_GUARANTEE, ):
            if guarantee is None:
                raise Exception('Params <guarantee> must be specified')

        self.group = group
        self.suggest_type = suggest_type
        self.msg = msg
        self.guarantee = guarantee

    @staticmethod
    def from_string(s):
        """Create suggest from string as <groupname:new_guarantee> (guarantee in gigabytes)"""
        groupname, _, new_guarantee = s.partition(':')
        group = CURDB.groups.get_group(groupname)
        new_guarantee = float(new_guarantee)
        return TMemorySuggest(group, TMemorySuggest.ESuggestType.MODIFY_GUARANTEE, 'Set instances guarantee to <{:.3f} Gb>'.format(new_guarantee),
                        guarantee=new_guarantee)

    def can_apply(self):
        """Check if we have changes to apply"""
        return self.suggest_type in (TMemorySuggest.ESuggestType.MODIFY_GUARANTEE,)

    def apply(self, update_db=False):
        """Apply changes to group"""
        if self.suggest_type not in (TMemorySuggest.ESuggestType.MODIFY_GUARANTEE,):
            raise Exception('Can not apply with suggest type <{}>'.format(self.suggest_type))

        self.group.card.reqs.instances.memory_guarantee = ByteSize('{:.3f} Gb'.format(self.guarantee))

        self.group.mark_as_modified()

        if update_db:
            CURDB.update(smart=True)


def suggested_close_to_current(old_guarantee, new_guarantee):
    """Check if new guarantee is close to old guarantee and we should not modify"""
    if math.fabs(old_guarantee - new_guarantee) < MAX_ABSOLUTE_DIFF:
        return True

    if min(old_guarantee, new_guarantee) / max(old_guarantee, new_guarantee) + MAX_RELATIVE_DIFF > 1:
        return True

    return False


def analyze_for_normal_group(group, clickhouse_result, quantile99=None):
    """Analyze for normal group"""

    assert clickhouse_result is not None or quantile99 is not None

    # calculate current and new guarantee
    if quantile99 is None:
        new_guarantee = calculate_quantile_for_normal_group(clickhouse_result)
    else:
        new_guarantee = quantile99
    current_guarantee = group.card.reqs.instances.memory_guarantee.value / 1024. / 1024 / 1024


    if suggested_close_to_current(current_guarantee, new_guarantee):
        msg = 'Do nothing: new guarantee <{:.2f} Gb> is close to old guarantee <{:.2f} Gb>'.format(new_guarantee, current_guarantee)
        return TMemorySuggest(group, TMemorySuggest.ESuggestType.NOTHING, msg, guarantee=new_guarantee)
    else:
        msg = 'Modify guarantee: <{:.2f} Gb> => <{:.2f} Gb>'.format(current_guarantee, new_guarantee)
        return TMemorySuggest(group, TMemorySuggest.ESuggestType.MODIFY_GUARANTEE, msg, guarantee=new_guarantee)


def suggest_for_normal_group(group, quantile99=None, force=False):
    """Make suggest for group with audit_class=normal"""
    if (not force) and group.card.properties.full_host_group:
        return TMemorySuggest(group, TMemorySuggest.ESuggestType.NOTHING, 'Full host group can not be optimized')

    if (not force) and group.card.properties.fake_group:
        return TMemorySuggest(group, TMemorySuggest.ESuggestType.NOTHING, 'Fake group can not be optimized')

    if quantile99 is None and len(group.get_kinda_busy_hosts()) == 0:
        return TMemorySuggest(group, TMemorySuggest.ESuggestType.NOTHING, 'Group is empty (has no hosts)')

    clickhouse_report = None
    if quantile99 is None:
        if get_capaview_report().get('usage_stats', {}).get(TClickhouseGroupReport.EPeriods.TWO_WEEKS, {}).get(group.card.name, None) is None:
            return TMemorySuggest(group, TMemorySuggest.ESuggestType.NOTHING, 'No data for group in clickhouse')

        clickhouse_report = TClickhouseGroupReport.from_usage_dict(get_capaview_report(), group, 'instance_memusage')

        status, suggest = have_enough_data(group, clickhouse_report, TMemorySuggest)
        if not status:
            return suggest

    return analyze_for_normal_group(group, clickhouse_result=clickhouse_report, quantile99=quantile99)


def suggest_for_group(group, quantile99=None, force=False):
    """Make suggestion for group

    :param group: gencfg group to work with
    :type group: core.igroups.IGroup
    :param quantile99: already calculated 99-median of usage (if not specified will be calculated)
    :type quantile99: float
    :param force: force calculate suggest (even for full_host/fake groups)
    :type force: book
    """

    return suggest_for_normal_group(group, quantile99=quantile99, force=force)
