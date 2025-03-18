"""
    Assign weight functions. When we build string for CgiSearchPrefix, we can group instances in different ways.
"""

from gaux.aux_collections import DefaultOrderedDict

from core.db import CURDB
from core.hosts import FakeHost
from core.instances import FakeInstance
import gaux.aux_portovm

from aux_utils import may_be_guest_instance


class TAssignWeightInstance(object):
    """
        This class describes gencfg instance with additional stuff, required to generate SearchSourceInfo string
    """

    __slots__ = ['instance', 'is_main', 'is_ssd_group', 'brigade_id', 'instance_power', 'tier', 'primus_list', 'multishard_id', 'intl2_group_id', 'shard_id']

    def __init__(self, instance, options, custom_port=None):
        notfound_keys = set(options.keys()) - set(self.__slots__)
        if len(notfound_keys) > 0:
            raise Exception("Keys <%s> not found while creating TAssignWeightInstance" % ",".join(notfound_keys))

        self.instance = may_be_guest_instance(instance, custom_port=custom_port)
        self.is_main = options.get('is_main', False)
        self.is_ssd_group = options.get('is_ssd_group', False)
        self.brigade_id = options.get('brigade_id',
                                      None)  # this option is used to distinguish instances from different brigades
        self.instance_power = options.get('instance_power',
                                          instance.power)  # this options is used to override instance power
        self.tier = options.get('tier', 'None')
        self.primus_list = options.get('primus_list', 'None')
        self.multishard_id = options.get('multishard_id', None)
        self.intl2_group_id = options.get('intl2_group_id', None)
        self.shard_id = options.get('shard_id', None)


class TAssignWeight(object):
    """
        This class convert list of instances [i1, i2, ..., iN] in something likes this:
            - ([(i1, [w1, w2, w3]), (i2, [w4, w5, w5]), ...], (wl1, wl2)), ...
            - (i1, [w1]), (i2, [w2]), ... (iN, [wN])
        Public interface contains functions, which get TSourceNodeInstance as input param and combine them in some way
    """

    def __init__(self):
        pass

    @classmethod
    def _group_instances_by_func(self, instances, func, attempts):
        """
            This function generate TAssignWeight structure, grouping instances with func <func>
        """

        grouped_instances = DefaultOrderedDict(list)
        for instance in instances:
            grouped_instances[func(instance)].append(instance)

        # group instances groups in somewhat stable order
        grouped_instances = grouped_instances.values()
        grouped_instances.sort(key=lambda x: tuple(y.instance.short_name() for y in x))

        result = []
        for lst in grouped_instances:
            lst_power = [sum(map(lambda x: max(1, int(x.instance_power * 100)), lst))]  # only one attempt at top level somehow
            lst_instances = map(lambda x: (x, [max(1, int(x.instance_power * 100))] * attempts), lst)

            result.append((lst_instances, lst_power))

        return result

    @classmethod
    def default(self, instances, params):
        """
            Default weight method. Generated flat structure

            :param instances(TSourceNodeInstance): list of instances
            :param params(dict): dict-like params with options like attempts,...
        """
        attempts = params.get('_attempts', 3)

        return map(lambda x: (x, [max(1, int(params.get('power', x.instance_power) * 100))] * attempts), instances)

    @classmethod
    def default_snippet(self, instances, params):
        """
            Default weight method for snippets. Group instances by brigade_id and set weight for brigade id as following:
                - @1@1@0@0@0 - if brigade is ssd
                - @0@0@1@1@1 - if brigade is not ssd
        """

        del params

        instances_by_brigade_id = DefaultOrderedDict(list)
        for instance in instances:
            instances_by_brigade_id[instance.brigade_id].append(instance)

        result = []
        for lst in instances_by_brigade_id.itervalues():
            if lst[0].is_ssd_group:
                weights = [1, 1, 0, 0, 0]
            else:
                weights = [0, 0, 1, 1, 1]
            result.append((map(lambda x: (x, [1]), lst), weights))

        return result

    @classmethod
    def only_ssd_snippet(self, instances, params):
        """
            Weight method for snippets, which drops nossd instances
        """
        instances = filter(lambda x: x.is_ssd_group, instances)
        return TAssignWeight.default_snippet(instances, params)

    @classmethod
    def group_by_queue(self, instances, params):
        """
            Make two-tier structure with first instances grouped by queue
        """

        attempts = params.get('_attempts', 3)

        return TAssignWeight._group_instances_by_func(instances, lambda x: x.instance.host.queue, attempts)

    @classmethod
    def group_by_dc(self, instances, params):
        """
            Make two-tier structure with first instances grouped by dc
        """

        attempts = params.get('_attempts', 3)

        return TAssignWeight._group_instances_by_func(instances, lambda x: x.instance.host.dc, attempts)

    @classmethod
    def group_by_location(self, instances, params):
        """
            Make two-tier structure with first instances grouped by location
        """

        attempts = params.get('_attempts', 3)

        return TAssignWeight._group_instances_by_func(instances, lambda x: x.instance.host.location, attempts)

    @classmethod
    def group_by_brigade(self, instances, params):
        attempts = params.get('_attempts', 3)
        return TAssignWeight._group_instances_by_func(instances, lambda x: x.brigade_id, attempts)

    @classmethod
    def group_by_host(self, instances, params):
        attempts = params.get('_attempts', 3)
        return TAssignWeight._group_instances_by_func(instances, lambda x: x.instance.host.name, attempts)

    @classmethod
    def default_int(self, instances, params):
        """
            Default weight method for ints. First attempt always goes to basesearch in same group, other attemps to all others according to weights
        """

        attempts = params.get('_attempts', 3)

        result = []
        for instance in instances:
            first_attempt_weights = max(1, int(instance.instance_power) * 100) if instance.is_main else 0
            weights = [first_attempt_weights] + [max(1, int(instance.instance_power * 100))] * (attempts - 1)
            result.append((instance, weights))

        return result

    @classmethod
    def cbir_group_by_dc(self, instances, params):
        """
            Weight method for cbir. First group instances by dc, than set first two attempts to go to current dc,
            other attempts go to remote dcs. E. g. our int in dc MSK and we have 3 hosts: msk, sas, man. Method
            should return something like ((msk, [w1]), [100, 100, 0]), ((sas, [w2]), [0, 0, 100]), ((man, [w3]), [0, 0, 100])
        """

        instances_by_location = DefaultOrderedDict(list)
        for instance in instances:
            instances_by_location[instance.instance.host.location].append(instance)

        result = []
        for location in sorted(instances_by_location.keys()):
            loc_instances = instances_by_location[location]
            if location == params['coordinator_instance'].host.location:
                weights = [100, 100, 0, 0, 0]
            else:
                weights = [0, 0, 100, 100, 100]
            result.append((TAssignWeight.default(loc_instances, params), weights))

        return result


def _generate_cgi_search_prefix_recurse(instances, weights, params):
    """
        Generate CgiSearchPrefix content
    """
    if isinstance(instances, TAssignWeightInstance):
        custom_port = params.get('custom_port')
        if not custom_port:
            port_offset = params.get('port_offset')
            if port_offset:
                custom_port = str(instances.instance.port + port_offset)

        generated_url = instances.instance.get_url(
            IPv=params.get('IPv', 6),
            lua_port_tpl=params.get('_lua_port_tpl', None),
            custom_port=custom_port,
            source_collection=params.get('_source_collection', 'yandsearch'),
            proto=params.get('_proto', 'http'),
        )
        return "%s%s" % (generated_url, "".join(map(lambda x: "@%s" % x, weights)))
    elif isinstance(instances, list):
        result = []
        dc = ''
        if params.get('_dc_group_required', False):
            for subinstances, subweights in instances:
                if not isinstance(subinstances, TAssignWeightInstance):
                    dc = ''
                    break
                if not dc:
                    dc = subinstances.instance.host.dc
                if dc != subinstances.instance.host.dc:
                    dc = ''
                    break
            if not dc:
                raise Exception("dc is empty but required")

        for subinstances, subweights in instances:
            result.append(_generate_cgi_search_prefix_recurse(subinstances, subweights, params))
        result = " ".join(result)
        return "(%s)%s%s" % (result, dc, "".join(map(lambda x: "@%s" % x, weights)))
    else:
        raise Exception("OOPS")


def generate_cgi_search_prefix(instances, params):
    if params.get('_dc_group_required') and any(map(lambda x: isinstance(x[0], TAssignWeightInstance), instances)):
        raise Exception("can't assign dc for flat grouped hosts")
    return ' '.join(_generate_cgi_search_prefix_recurse(instance, weights, params) for instance, weights in instances)
