import re
import math

try:
    from gaux.aux_utils import get_worst_tier_by_size
except:
    pass


I1G = int(1024 * 0.95)  # 1Gb of physical memory is something between 950Mb and 980Mb of available virtual



class IBaseFuncs(object):
    @staticmethod
    def msk_web_production_slave_slots(db, host):
        hostgroups = set(map(lambda x: x.card.name, db.groups.get_host_groups(host)))

        N = 0
        if 'MSK_WEB_BASE_PIP' in hostgroups:
            N += 1
        if 'SAS_WEB_BASE_PIP' in hostgroups:
            N += 1
        if 'SAS_ARCNEWS_BASE' in hostgroups:
            N += 1
        if 'SAS_ARCNEWS_STORY_BASE' in hostgroups:
            N += 1
        if 'SAS_WEB_MMETA_NEW' in hostgroups:
            N += 3
        if 'MSK_DIVERSITY2_BASE_PIP' in hostgroups:
            N += 1
        if 'MSK_BLOGS_BASE' in hostgroups:
            N += 1
        if 'MSK_ARCNEWS_BASE' in hostgroups:
            N += 1
        if 'MSK_ARCNEWS_STORY_BASE' in hostgroups:
            N += 1

        if db.version < '2.2.4':
            if 'SAS_VM' in hostgroups:
                N += 1

        if 'MSK_WEB_RRG_BASE_PIP' in hostgroups:
            N += 2
        if 'SAS_WEB_BASE_BACKUP' in hostgroups:
            N += 1
        if 'MSK_PERSONAL_HISTORY_BASE_NEW' in hostgroups:
            N += 1
        if 'MSK_SOCIAL_TUR_BASE_NEW' in hostgroups:
            N += 2
        if 'MSK_SOCIAL_RUS_BASE_NEW' in hostgroups:
            N += 2
        if 'MSK_MUSICMATCH_BASE_NEW' in hostgroups:
            N += 1
        if 'MSK_DIRECT_FASTTIER_BASE_PRIEMKA_NEW' in hostgroups:
            N += 2
        if 'MSK_WEB_BASE_PIP_BACKUP' in hostgroups:
            N += 1
        if 'MSK_WEB_ITALY_BASE' in hostgroups:
            N += 2
        if 'SAS_SOCIAL_TUR_BASE_NEW' in hostgroups:
            N += 2
        if 'SAS_SOCIAL_RUS_BASE_NEW' in hostgroups:
            N += 2
        if 'MSK_WIKIFACTS_COM_NEW' in hostgroups:
            N += 1
        if 'SAS_WIKIFACTS_COM_NEW' in hostgroups:
            N += 1
        if 'SAS_PERSONAL_HISTORY_BASE_NEW' in hostgroups:
            N += 1
        if 'SAS_RUSMAPS_BASE_NEW' in hostgroups:
            N += 1
        if 'SAS_TURMAPS_BASE_NEW' in hostgroups:
            N += 1
        if 'SAS_NEWS_FEEDFINDER_NEW' in hostgroups:
            N += 1
        if 'SAS_SITESUGGEST_BASE_NEW' in hostgroups:
            N += 1
        if 'MSK_MUSICMIC_BASE' in hostgroups:
            N += 1
        if 'MSK_WEB_RRG_BASE_PIP_BACKUP' in hostgroups:
            N += 2
        if 'MAN_QUERYSEARCH_USERDATA_PIP' in hostgroups:
            N += 2
        if 'SAS_PERSONAL_HISTORY_SSD_BASE' in hostgroups:
            N += 1
        if 'MAN_PERSONAL_HISTORY_SSD_BASE' in hostgroups:
            N += 1
        if 'MSK_PERSONAL_HISTORY_SSD_BASE' in hostgroups:
            N += 1
        if 'SAS_RUSMAPS_BASE' in hostgroups:
            N += 1
        if 'SAS_TURMAPS_BASE' in hostgroups:
            N += 1
        if 'SAS_KIWICALC_LOAD_PORTOVM' in hostgroups:
            N += 1
        if 'SAS_KIWICALC_NOLOAD_PORTOVM' in hostgroups:
            N += 1

        if db.version >= '2.1.1':
            if 'MSK_WEB_BASE_COMTR' in hostgroups:
                N += 1

        if db.version != '2.0' and db.version != '2.0.1' and db.version != '0.9.5':  # something strange
            if 'SAS_KIWICALC_NEW_VM' in hostgroups:
                N += 1

        # legacy groups (in previous tags)
        if db.version <= '2.0.1':
            if 'MSK_WEB_GER_BASE' in hostgroups:  # stable53/r1
                N += 1
            if 'MSK_WEB_BASE_COMTRBACKUP' in hostgroups:  # stable53/r1
                N += 1
            if 'SAS_WEB_BASE_COMTRBACKUP' in hostgroups:  # stable53/r1
                N += 1
            if db.version != '0.3':
                if 'SAS_KIWICALC_KVM_EXPERIMENT' in hostgroups:
                    N += 1
            if 'SAS_KIWICALC_VM' in hostgroups:
                N += 1
            if 'SAS_KIWICALC2_VM' in hostgroups:
                N += 1
            if 'SAS_VPDELTA_TEST_VM' in hostgroups:
                N += 1
        return N

    @staticmethod
    def is_host_in_group(db, group_name, host):
        if db.groups.has_group(group_name):
            return db.groups.get_group(group_name).hasHost(host)
        return False

    class InstanceCountFuncs(object):
        MEXACT = re.compile('exactly(\d+)')
        MSIZE = re.compile('i(\d+)g')
        MDISK = re.compile('disk(\d+)')

        def __init__(self):
            pass

        def __getattr__(self, name):
            m = re.match(IBaseFuncs.InstanceCountFuncs.MEXACT, name)
            if m:
                return lambda x, y: int(m.group(1))

            m = re.match(IBaseFuncs.InstanceCountFuncs.MSIZE, name)
            if m:
                return lambda x, y: y.memory / int(m.group(1))

            m = re.match(IBaseFuncs.InstanceCountFuncs.MDISK, name)
            if m:
                return lambda x, y: y.disk / int(m.group(1)) if y.ssd == 0 else y.ssd / int(m.group(1))

            raise AttributeError("Unknown function name \"%s\"" % name)

        @staticmethod
        def default(db, host):
            del db, host
            return 1

        @staticmethod
        def production_yt_rtc(db, host):
            # check memory used by all slave instances
            used_memory = 0.0
            hostgroups = set(map(lambda x: x, db.groups.get_host_groups(host)))
            for hostgroup in hostgroups:
                if hostgroup.card.name in ['VLA_WEB_PLATINUM_JUPITER_BASE', 'VLA_WEB_TIER0_JUPITER_BASE', 'VLA_WEB_TIER1_JUPITER_BASE', 'VLA_WEB_CALLISTO_CAM_BASE',
                                           'VLA_WEB_PLATINUM_JUPITER_INT', 'VLA_WEB_TIER0_JUPITER_INT', 'VLA_WEB_TIER1_JUPITER_INT', 'VLA_WEB_CALLISTO_CAM_INT',
                                           'SAS_WEB_PLATINUM_JUPITER_BASE', 'SAS_WEB_TIER0_JUPITER_BASE', 'SAS_WEB_TIER1_JUPITER_BASE', 'SAS_WEB_CALLISTO_CAM_BASE',
                                           'SAS_WEB_PLATINUM_JUPITER_INT', 'SAS_WEB_TIER0_JUPITER_INT', 'SAS_WEB_TIER1_JUPITER_INT', 'SAS_WEB_CALLISTO_CAM_INT',
                                           'MAN_WEB_PLATINUM_JUPITER_BASE', 'MAN_WEB_TIER0_JUPITER_BASE', 'MAN_WEB_TIER1_JUPITER_BASE', 'MAN_WEB_CALLISTO_CAM_BASE',
                                           'MAN_WEB_PLATINUM_JUPITER_INT', 'MAN_WEB_TIER0_JUPITER_INT', 'MAN_WEB_TIER1_JUPITER_INT', 'MAN_WEB_CALLISTO_CAM_INT',
                                          ]:
                    continue
                iperhost = 1
                if hostgroup.card.legacy.funcs.instanceCount.startswith('exactly'):
                    iperhost = int(hostgroup.card.legacy.funcs.instanceCount[7:])
                used_memory += iperhost * hostgroup.card.reqs.instances.memory_guarantee.value
            used_memory /= float(2 ** 30)
            left_memory = host.get_avail_memory() / 1024. / 1024 / 1024 - used_memory

            N = int(left_memory / 20)
            N = max(0, N)

            return N

        @staticmethod
        def productionvla_v2(db, host):
            # check memory used by all slave instances
            used_memory = 0.0
            hostgroups = set(map(lambda x: x, db.groups.get_host_groups(host)))
            for hostgroup in hostgroups:
                if hostgroup.card.name in ['VLA_WEB_BASE', 'VLA_WEB_TIER0_JUPITER_BASE', 'VLA_WEB_TIER1_JUPITER_BASE', 'VLA_WEB_PLATINUM_JUPITER_BASE',
                                           'VLA_WEB_CALLISTO_CAM_BASE', 'VLA_VIDEO_BASE', 'VLA_IMGS_BASE']:
                    continue

                iperhost = 1
                if hostgroup.card.legacy.funcs.instanceCount.startswith('exactly'):
                    iperhost = int(hostgroup.card.legacy.funcs.instanceCount[7:])
                used_memory += iperhost * hostgroup.card.reqs.instances.memory_guarantee.value

            used_memory /= float(2 ** 30)
            left_memory = host.memory - used_memory

            return max(min(20, int(left_memory / 27)), 1)

        @staticmethod
        def video_with_slaves(db, host):
            # check memory used by all slave instances
            used_memory = 0.0
            hostgroups = set(map(lambda x: x, db.groups.get_host_groups(host)))
            for hostgroup in hostgroups:
                if hostgroup.card.name in ['MAN_VIDEO_BASE', 'SAS_VIDEO_BASE', 'MSK_VIDEO_BASE', 'SAS_WEB_PLATINUM_BASE_BACKGROUND']:
                    continue

                iperhost = 1
                if hostgroup.card.legacy.funcs.instanceCount.startswith('exactly'):
                    iperhost = int(hostgroup.card.legacy.funcs.instanceCount[7:])
                used_memory += iperhost * hostgroup.card.reqs.instances.memory_guarantee.value

            used_memory /= float(2 ** 30)
            left_memory = host.memory - used_memory

            return int(left_memory / 44)


    MEXACT = re.compile('exactly(\d+)')
    MCORES = re.compile('(\d+(?:\.\d+)?)c')  # function that specifies number of cores

    @staticmethod
    def gen_instance_power_func(name):
        m = re.match(IBaseFuncs.MEXACT, name)
        if m:
            return lambda x, y, N: [float(m.group(1))] * N

        m = re.match(IBaseFuncs.MCORES, name)
        if m:
            return lambda db, host, N: [(host.power / db.cpumodels.get_model(host.model).ncpu) * float(m.group(1))] * N

        if name in ('default', 'fullhost'):
            return lambda db, host, N: [host.power / N] * N if N > 0 else []
        elif name == 'zero':
            return lambda db, host, N: [0.] * N
        elif name == 'newmisc1g':
            return lambda db, host, N: [host.power * 1 / host.memory] * N
        elif name == 'newmisc2g':
            return lambda db, host, N: [host.power * 2 / host.memory] * N
        elif name == 'newmisc3g':
            return lambda db, host, N: [host.power * 3 / host.memory] * N
        elif name == 'newmisc5g':
            return lambda db, host, N: [host.power * 5 / host.memory] * N

        return AttributeError('Unknown function name {}'.format(name))

    @staticmethod
    def get_instance_port_func(name):
        if name.startswith('new'):
            # some instances types bind on more than a single port
            # we reserve 8 ports and assume that instance takes continious range of port
            # starting from given port number
            start_port = int(name[3:])
            return lambda x: start_port + 8 * x
        elif name.startswith('old'):
            start_port = int(name[3:])
            return lambda x: start_port + x
        elif name == 'default':
            return lambda x: 8041 + x
        else:
            raise AttributeError('Unknown port func name {}'.format(name))

    @classmethod
    def create_instance_count_func(cls, group):
        # function for automated groups
        if group.reqs is None:
            raise Exception('Cannot create instance count func for group "%s". Group "reqs" section is missing' % group.card.name)
        reqs = group.reqs

        constraint_funcs = []
        if reqs.instances.memory.megabytes() != 0:
            constraint_funcs.append(
                lambda host, memory_per_i=reqs.instances.memory.gigabytes(): int(host.memory / memory_per_i))
        if reqs.instances.power != 0:
            constraint_funcs.append(lambda host, power_per_i=reqs.instances.power: int(host.power / power_per_i))
        if reqs.instances.disk.gigabytes() != 0:
            constraint_funcs.append(
                lambda host, disk_per_i=reqs.instances.disk.gigabytes(): int((host.disk - 20) / disk_per_i))
        if reqs.instances.ssd.gigabytes() != 0:
            constraint_funcs.append(lambda host, ssd_per_i=reqs.instances.ssd.gigabytes(): int(host.ssd / ssd_per_i))

        max_per_host = reqs.instances.max_per_host
        min_per_host = reqs.instances.min_per_host
        if not constraint_funcs and not max_per_host:
            raise Exception('Group %s has no upper limit for instances on host.' % group.card.name)

        def result_function(db, host):
            del db
            max_limits = []
            max_limits.extend([func(host) for func in constraint_funcs])
            if max_per_host:
                max_limits.append(max_per_host)
            assert max_limits
            max_limit = min(max_limits)
            if min_per_host and max_limit < min_per_host:
                max_limit = 0
            return max_limit

        return result_function

    def __init__(self, group):
        self.instanceCount = getattr(IBaseFuncs.InstanceCountFuncs(), group.card.legacy.funcs.instanceCount)
        self.instancePower = IBaseFuncs.gen_instance_power_func(group.card.legacy.funcs.instancePower)
        self.instancePort = IBaseFuncs.get_instance_port_func(group.card.legacy.funcs.instancePort)

    @staticmethod
    def check(group):
        m = {
            'instanceCount': 'InstanceCountFuncs',
            'instancePower': 'InstancePowerFuncs',
            'instancePort': 'InstancePortFuncs',
        }
        for key, value in group.legacy.funcs.items():
            cls = getattr(IBaseFuncs, m[key])
            cls_instance = cls()
            if getattr(cls_instance, value, None) is None:
                raise Exception('Key "%s" has bad value "%s"' % (key, value))
