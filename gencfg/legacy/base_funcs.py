import re
import math

from gaux.aux_utils import get_worst_tier_by_size

I1G = int(1024 * 0.95)  # 1Gb of physical memory is something between 950Mb and 980Mb of available virtual


class IBaseFuncs(object):
    @staticmethod
    def msk_web_production_slave_slots(db, host):
        hostgroups = set(map(lambda x: x.name, db.groups.get_host_groups(host)))

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
        if 'MSK_WEB_SNIPPETS_SSD_BASE' in hostgroups:
            N += 2
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
        def __init__(self):
            pass

        MEXACT = re.compile('exactly(\d+)')
        MSIZE = re.compile('i(\d+)g')
        MDISK = re.compile('disk(\d+)')

        def __getattr__(self, name):
            m = re.match(IBaseFuncs.InstanceCountFuncs.MEXACT, name)
            if m:
                return lambda x, y: int(m.group(1))

            m = re.match(IBaseFuncs.InstanceCountFuncs.MSIZE, name)
            if m:
                return lambda x, y: y.memory / int(m.group(1))

            m = re.match(IBaseFuncs.InstanceCountFuncs.MDISK, name)
            if m:
                return lambda x, y: y.disk / int(m.group(1))

            raise AttributeError("Unknown function name \"%s\"" % name)

        @staticmethod
        def default(db, host):
            del db, host
            return 1

        @staticmethod
        def i1_5g(db, host):
            if db.version < '1.0':
                return int(host.memory / 1.46)
            else:
                return int(host.memory / 1.5)

        @staticmethod
        def querysearch(db, host):
            del db
            return max(min(host.memory / 12, (host.disk - 60) / 80), 1)

        @staticmethod
        def querysearchssd(db, host):
            del db
            return (host.disk - 60) / 100

        @staticmethod
        def priemka_backup(db, host):
            del db
            n_by_memory = host.memory / 16
            n_by_disk = (host.disk - 20) / 96
            if n_by_disk - n_by_memory > 7:
                return 7
            return 0

        @staticmethod
        def priemka_backup_small(db, host):
            del db
            n_by_memory = host.memory / 16
            n_by_disk = (host.disk - 20) / 96
            return min(n_by_disk - n_by_memory, 7)

        @staticmethod
        def priemka(db, host):
            n = host.memory / 16

            if IBaseFuncs.is_host_in_group(db, 'MSK_WEB_BASE_PRIEMKA_BACKUP', host):
                n -= 1
            elif IBaseFuncs.is_host_in_group(db, 'MSK_DIVERSITY2_BASE_PRIEMKA_BACKUP', host):
                n -= 1

            if n < 0:
                raise Exception("Non-positive number of priemka instances for host %s" % host.name)
            return n

        @staticmethod
        def r1_backup(db, host):
            del db
            n_by_memory = host.memory / 12
            n_by_disk = (host.disk - 100) / 74
            if n_by_disk - n_by_memory > 7:
                return 7
            return 0

        @staticmethod
        def r1(db, host):
            n_by_memory = host.memory / 12
            n_by_disk = (host.disk - 100) / 96
            n = min(n_by_memory, n_by_disk)

            if IBaseFuncs.is_host_in_group(db, 'MSK_WEB_RESEARCH_BASE_R1', host):
                n -= 1
            if IBaseFuncs.is_host_in_group(db, 'MSK_WEB_BASE_R1_BACKUP', host):
                n -= 1
            elif IBaseFuncs.is_host_in_group(db, 'MSK_DIVERSITY2_BASE_R1_BACKUP', host):
                n -= 1

            if db.version <= '2.0.1':
                if IBaseFuncs.is_host_in_group(db, 'MSK_RUSMAPS_BASE_R1_BACKUP', host):
                    n -= 1
                if IBaseFuncs.is_host_in_group(db, 'MSK_TURMAPS_BASE_R1_BACKUP', host):
                    n -= 1

                #            if n_by_memory == 1 and n == 1:
                #                raise Exception("Host %s with only one shard in R1" % host.name)
            if n == 0 and IBaseFuncs.is_host_in_group(db, 'MSK_WEB_BASE_R1_BACKUP', host):
                raise Exception("Host %s with only backup replicas: %s by memory, %s by disk" % (host.name, n_by_memory, n_by_disk))

            return n

        @staticmethod
        def production24(db, host):
            del db
            if host.memory < 24:
                return 1
            return host.memory / 24

        @staticmethod
        def productionsas(db, host):
            N = host.memory / 12 - db.base_funcs.msk_web_production_slave_slots(db, host)

            if db.version <= '0.2.5':
                N = min(N, 6)

            if db.version >= '2.1.2':
                N_by_disk = int((host.disk - 60) / get_worst_tier_by_size(
                    "RRGTier0:2,PlatinumTier0:1,UkrTier0:1,EngTier0:1,EngTier1:1,TurTier0:1,UkrTier1:1,KzTier0:1,PlatinumTurTier0:1",
                    db) / 2)
                N = min(N, N_by_disk)

            if N < 0:
                raise Exception("Host %s with %s production instances" % (host.name, N))

            hostgroups = set(map(lambda x: x.name, db.groups.get_host_groups(host)))
            if 'MSK_WEB_RRG_BASE' in hostgroups:  # temporary hack
                return 0

            return N

        @staticmethod
        def production(db, host):
            N = host.memory / 12 - db.base_funcs.msk_web_production_slave_slots(db, host)

            if db.version <= '0.2.5':
                N = min(N, 6)

            if db.version >= '2.1.2':
                N_by_disk = int((host.disk - 60) / get_worst_tier_by_size(
                    "RRGTier0:2,PlatinumTier0:1,UkrTier0:1,EngTier0:1,EngTier1:1,TurTier0:1,UkrTier1:1,KzTier0:1,PlatinumTurTier0:1",
                    db) / 2)
                N = min(N, N_by_disk)

            if N < 0:
                raise Exception("Host %s with %s production instances" % (host.name, N))

            if db.version > '2.1':
                hostgroups = set(map(lambda x: x.name, db.groups.get_host_groups(host)))
                if 'MSK_WEB_RRG_BASE' in hostgroups:  # temporary hack
                    return 0

            return N

        @staticmethod
        def rrg(db, host):
            N = host.memory / 12 - db.base_funcs.msk_web_production_slave_slots(db, host)
            return N / 2

        @staticmethod
        def sasweb(db, host):
            if db.version <= '0.2':
                return host.memory / 12
            else:
                return host.memory / 16

        @staticmethod
        def snippets_ssd(db, host):
            if db.version >= '2.2':
                if db.version >= '2.2.1':
                    if host.ssd > 1000:
                        return 16
                    else:
                        return 9
                else:
                    return 9
            else:
                return 12
                # return host.ssd / 32

        @staticmethod
        def tur1(db, host):
            del db
            return 0 if host.disk / 100 < 8 else 8

        @staticmethod
        def imgs_quick(db, host):
            del db
            return 0 if host.memory / 10 == 0 else 5

        @staticmethod
        def jud0(db, host):
            N = host.memory / 12
            if IBaseFuncs.is_host_in_group(db, 'MSK_JUD_BASE_PIP', host):
                N -= 1
            return N

        @staticmethod
        def imtub(db, host):
            if db.version <= '2.0.1':
                return host.disk / 3600
            else:
                return host.disk / 1700

        @staticmethod
        def fusion(db, host):
            if db.version <= '2.0':
                return (host.memory - 20) / 23
            else:
                return (host.memory - 20) / 22

        @staticmethod
        def fusion2(db, host):
            del db
            if host.memory >= 64:
                return 2
            if host.memory >= 32:
                return 1
            return 0

        @staticmethod
        def fusion64(db, host):
            del db
            if host.memory >= 128:
                return 2
            if host.memory >= 64:
                return 1
            return 0

        @staticmethod
        def fusionimgs(db, host):
            del db
            if host.memory >= 48:
                return 2
            if host.memory >= 24:
                return 1
            return 0

    class InstancePowerFuncs(object):
        def __init__(self):
            pass

        @staticmethod
        def default(db, host, N):
            del db
            if N == 0:
                return []
            return [host.power / N] * N

        @staticmethod
        def zero(db, host, N):
            del db, host
            return [1.] * N

        @staticmethod
        def misc(db, host, N):
            del db
            n_low = 3 * N / 4
            n_high = N - n_low

            return [host.power / 4. / n_low] * n_low + [3 * host.power / 4 / n_high] * n_high

        @staticmethod
        def backup(db, host, N):
            del db
            return [host.power] * N

        @staticmethod
        def newmisc1g(db, host, N):
            del db
            return [host.power * 1 / host.memory] * N

        @staticmethod
        def newmisc2g(db, host, N):
            del db, N
            return [host.power * 2 / host.memory]

        @staticmethod
        def newmisc3g(db, host, N):
            del db, N
            return [host.power * 3 / host.memory]

        @staticmethod
        def newmisc4g(db, host, N):
            del db, N
            return [host.power * 4 / host.memory]

        @staticmethod
        def newmisc5g(db, host, N):
            del db, N
            return [host.power * 5 / host.memory]

        @staticmethod
        def newmisc6g(db, host, N):
            del db, N
            return [host.power * 6 / host.memory]

        @staticmethod
        def newmisc9g(db, host, N):
            del db, N
            return [host.power * 9 / host.memory]

        @staticmethod
        def newmisc10g(db, host, N):
            del db, N
            return [host.power * 9 / host.memory]

        @staticmethod
        def newmisc12g(db, host, N):
            del db, N
            return [host.power * 12 / host.memory]

        @staticmethod
        def newmisc15g(db, host, N):
            del db, N
            return [host.power * 15 / host.memory]

        @staticmethod
        def newmisc16g(db, host, N):
            del db, N
            return [host.power * 16 / host.memory]

        @staticmethod
        def r1(db, host, N):  # FIXME
            if IBaseFuncs.is_host_in_group(db, 'MSK_WEB_RESEARCH_BASE_R1', host):
                return [host.power / (N + 1)] * N
            return [host.power / N] * N

        @staticmethod
        def mskweb(db, host, N):
            if db.version <= '0.3':
                LOW_MAP = {
                    'E5530': 6,
                    'E5620': 6,
                    'E5645': 6,
                    'E5-2660': 5,
                }

                ncpu = db.cpumodels.get_model(host.model).ncpu
                if ncpu < N:
                    raise Exception("Can not generate %s instances for host %s (mskweb)" % (N, host.name))

                host_power = host.power
                if IBaseFuncs.is_host_in_group(db, 'MSK_WEB_SNIPPETS_SSD_BASE', host):
                    host_power -= 240.

                if db.version <= '0.1':
                    low_n = LOW_MAP.get(host.model, 6)
                    if low_n * (N - 1) + 1 < ncpu:
                        return [host_power * (ncpu - low_n * (N - 1)) / ncpu] + [host_power * low_n / ncpu] * (N - 1)
                    else:
                        power = []
                        for i in range(N):
                            fst = ncpu * i / N
                            lst = ncpu * (i + 1) / N
                            assert (lst > fst)
                            power.append(host_power * (lst - fst) / ncpu)
                        return power
                else:
                    if N * 100. > host_power:
                        return [host_power / N] * N
                    else:
                        return [100.] * (N - 1) + [host_power - 100. * (N - 1)]
            else:
                host_power = host.power
                if IBaseFuncs.is_host_in_group(db, 'MSK_WEB_SNIPPETS_SSD_BASE', host):
                    host_power -= 240.
                if N > 0:
                    return [host_power / N] * N
                else:
                    return []

        @staticmethod
        def mskpriemkainproduction(db, host, N):
            del db
            if N != 1:
                raise Exception("More than one instances in MSK_PIP on host %s" % host.name)
            return [1.]

        @staticmethod
        def msk_ams_backup(db, host, N, loc='MSK'):
            del db
            if N != 1:
                raise Exception("More than one instances in %s_WEB_BASE_COMTRBACKUP on host %s" % (loc, host.name))
            return [math.log(host.power, 2.0)]

        @staticmethod
        def sas_ams_backup(db, host, N):
            return IBaseFuncs.InstancePowerFuncs.msk_ams_backup(db, host, N, loc='SAS')

        @staticmethod
        def sasweb(db, host, N):
            del db
            return [610.] + [(host.power - 610.) / (N - 1)] * (N - 1)

        #            return [host.power / N] * N
        @staticmethod
        def oxygen(db, host, N):
            del db, host
            return [100.0] * N

        @staticmethod
        def all_equal_to_host_power(db, host, N):
            del db
            return [host.power] * N

        @staticmethod
        def amsweb(db, host, N):
            del db
            if N == 8:
                return [120., 120.] + [(host.power - 240.) / (N - 2)] * (N - 2)
            else:
                return [host.power / N] * N

        @staticmethod
        def divide_equally(db, host, N):
            del db
            # same as default
            if N == 0:
                return []
            return [host.power / N] * N

        @staticmethod
        def imtub(db, host, N):
            del db, host
            if N > 0:  # FIXME: remove this or check for no host with zero instances
                return [1000. / N] * N
            else:
                return []

    class InstancePortFuncs(object):
        def __init__(self):
            pass

        def __getattr__(self, name):
            if name.startswith('old'):
                start_port = int(name[3:])
                return lambda x: start_port + x
            if name.startswith('new'):
                # some instances types bind on more than a single port
                # we reserve 8 ports and assume that instance takes continious range of port
                # starting from given port number
                start_port = int(name[3:])
                return lambda x: start_port + 8 * x

            raise AttributeError("Unknown function name \"%s\"" % name)

        @staticmethod
        def default(N):
            return 8041 + N

    @classmethod
    def create_instance_count_func(cls, group):
        # function for automated groups
        if group.reqs is None:
            raise Exception('Cannot create instance count func for group "%s". Group "reqs" section is missing' % group.name)
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

        custom_instance_count_func = reqs.instances.custom_instance_count_func
        max_per_host = reqs.instances.max_per_host
        min_per_host = reqs.instances.min_per_host
        if not constraint_funcs and not custom_instance_count_func and not max_per_host:
            raise Exception('Group %s has no upper limit for instances on host.' % group.name)

        def result_function(db, host):
            max_limits = []
            max_limits.extend([func(host) for func in constraint_funcs])
            if max_per_host:
                max_limits.append(max_per_host)
            if custom_instance_count_func:
                max_limits.append(custom_instance_count_func(host))
            assert max_limits
            max_limit = min(max_limits)
            if min_per_host and max_limit < min_per_host:
                max_limit = 0
            return max_limit

        return result_function

    def __init__(self, group):
        self.instanceCount = getattr(IBaseFuncs.InstanceCountFuncs(), group.legacy.funcs.instanceCount)
        self.instancePower = getattr(IBaseFuncs.InstancePowerFuncs(), group.legacy.funcs.instancePower)
        self.instancePort = getattr(IBaseFuncs.InstancePortFuncs(), group.legacy.funcs.instancePort)

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
