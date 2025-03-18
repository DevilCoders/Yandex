"""
    Classes with triggers performed on some group update actions. Some triggers can be set manually by group owner, others run automatically, based on
    group information (e. g. trigger on portovm instances)
      - triggers, performed when adding host
      - triggers, performed when removing host
"""

import gaux.aux_decorators
from gaux.aux_portovm import gen_guest_group_name, gen_guest_group, gen_vm_host_name, gen_vm_host

from core.instances import TIntGroup, TMultishardGroup, TIntl2Group
from core.card.types import ByteSize
from core.settings import SETTINGS


class TGroupTriggers(object):
    def __init__(self):
        pass

    class ITrigger(object):
        def __init__(self):
            pass

        """
            Do nothing.
        """

        @staticmethod
        @gaux.aux_decorators.static_var("description", "Do nothing")
        def nothing(*args):
            pass

    class TOnAddHostsTriggers(object):
        """
            Trigger, called every time group.addHost called
        """

        def __init__(self):
            pass

        class TManualTriggers(object):
            """
                Manual triggers (user can specify on of them in group card
            """

            def __init__(self):
                pass

            @staticmethod
            @gaux.aux_decorators.static_var("description",
                                           "Distribute instances of of added hosts uniformly among all shards (firstly add to shards with less power)")
            def default(group, host):
                """
                    Default trigger. Distribute instances of of added hosts uniformly among all shards (firstly add to shards with less power).
                    Does not work with:
                        - groups with multiple intlookups;
                        - intlookup with multiple hosts per group;
                        - intlookup with multiple tiers;
                        - intlookup with ints;
                """
                def append_to_intsearchers(host, group, intlookup):
                    int_groups = intlookup.get_int_groups()
                    new_instances = group.get_host_instances(host)
                    for instance in new_instances:
                        int_groups.sort(key=lambda x: len(x.intsearchers))
                        int_groups[0].intsearchers.append(instance)

                def append_to_basesearchers(host, group, intlookup):
                    """Append host to basesearchers"""
                    if intlookup.hosts_per_group == 1:  # add hosts to brigade groups with less power
                        brigade_groups_with_power = map(lambda x: (sum(map(lambda y: y.power, x.brigades)), x.brigades),
                                                        intlookup.get_multishards())
                        brigade_groups_with_power.sort(cmp=lambda (x1, y1), (x2, y2): cmp(x1, x2))
                        instances = group.get_host_instances(host)

                        N = min(len(instances), len(brigade_groups_with_power))
                        instances = instances[:N]
                        brigade_groups_with_power = map(lambda (x, y): y, brigade_groups_with_power[:N])

                        for instance, lst in zip(instances, brigade_groups_with_power):
                            group = TIntGroup([[instance]], [])
                            lst.append(group)
                    else:  # fill the gaps
                        instances = group.get_host_instances(host)
                        for int_group in intlookup.get_int_groups():
                            for lst in int_group.basesearchers:
                                if len(lst) == 0:
                                    lst.append(instances.pop())
                                    if len(instances) == 0:
                                        break
                            if len(instances) == 0:
                                break

                if len(group.card.intlookups) == 0:
                    return
                elif len(group.card.intlookups) == 1:
                    intlookup = group.parent.db.intlookups.get_intlookup(group.card.intlookups[0])

                    if group.card.name == intlookup.base_type:
                        append_to_basesearchers(host, group, intlookup)
                    elif (len(intlookup.get_int_instances()) > 0) and (intlookup.get_int_instances()[0].type == group.card.name):
                        append_to_intsearchers(host, group, intlookup)

                    intlookup.mark_as_modified()
                else:
                    pass

            @staticmethod
            @gaux.aux_decorators.static_var("description", "Do nothing on adding host")
            def nothing(group, host):
                """
                    Nothing to do
                """
                pass

            @staticmethod
            @gaux.aux_decorators.static_var("description",
                                           "Add instances in golovan yasm style: create 2 replicas (increase number of shards if needed)")
            def yasm(group, host):
                """
                    Sequence of actionst to do:
                        - scan intlookup, find all shards with 1 or less replicas. If do not have one, add one extra shard;
                        - Add instance to first shard with not enough instances
                """
                mydb = group.parent.db


                if len(group.card.intlookups) == 0:
                    intlookup = mydb.intlookups.create_empty_intlookup(group.card.name)
                    intlookup.base_type = group.card.name
                    intlookup.hosts_per_group = 1
                    intlookup.brigade_groups_count = 0
                    intlookup.intl2_groups.append(TIntl2Group())
                    group.card.intlookups.append(intlookup.file_name)
                elif len(group.card.intlookups) == 1:
                    intlookup = mydb.intlookups.get_intlookup(group.card.intlookups[0])
                else:
                    assert len(group.card.intlookups) == 1, "Group <%s> should have exactly 1 intlookup" % group.card.name

                assert intlookup.hosts_per_group == 1, "Intlookup <%s> have <%d> hosts per group when should have <1>" % intlookup.hosts_per_group

                brigade_groups = filter(lambda x: len(x.brigades) <= 1, intlookup.get_multishards())
                if len(brigade_groups) > 0:
                    brigade_group = brigade_groups[-1]
                else:
                    brigade_group = TMultishardGroup()
                    intlookup.intl2_groups[-1].multishards.append(brigade_group)
                    intlookup.brigade_groups_count += 1

                brigade_group.brigades.append(TIntGroup([[group.get_host_instances(host)[0]]], []))

                intlookup.mark_as_modified()

        class TAutoTriggers(object):
            """
                Triggers, performed automatically based on group parameters
            """

            def __init__(self):
                pass

        @staticmethod
        def run(group, host):
            """
                Selector function. Select correct trigger from <triggers.on_add_host.method> from group card
            """
            # run manual triggers
            func = getattr(TGroupTriggers.TOnAddHostsTriggers.TManualTriggers, group.card.triggers.on_add_host.method, None)
            if func is None:
                raise Exception("OnAddHost trigger <%s> not found (group %s, host %s)" % (group.card.triggers.on_add_host.method, group.card.name, host.name))
            func(group, host)

    class TOnRemoveHostsTriggers(object):
        """
            Trigger, called every time group.removeHost called
        """

        def __init__(self):
            pass

        class TManualTriggers(object):
            """
                Manual triggers (user can specify on of them in group card
            """

            def __init__(self):
                pass

        class TAutoTriggers(object):
            """
                Triggers, performed automatically based on group parameters
            """

            def __init__(self):
                pass

            @staticmethod
            def portovm(host_group, host):
                mydb = host_group.parent.db

                if not mydb.groups.has_group(gen_guest_group_name(host_group.card.name)):
                    return

                guest_group = mydb.groups.get_group(gen_guest_group_name(host_group.card.name))
                for instance in host_group.get_host_instances(host):
                    guest_host = mydb.hosts.get_host_by_name(gen_vm_host_name(mydb, instance))

                    guest_group.removeHost(guest_host)
                    mydb.hosts.remove_host(guest_host)

        @staticmethod
        def run(group, host):
            """
                Selector function. Select correct triggers from group.card
            """

            # run triggers for portovm
            if group.card.tags.itype == "portovm":
                TGroupTriggers.TOnRemoveHostsTriggers.TAutoTriggers.portovm(group, host)

    class TOnSetItypeTriggers(object):
        """
            Trigger, called every time group.set_itype called
        """

        def __init__(self):
            pass

        class TAutoTriggers(object):
            """
                Triggers, performed automatically, based on group parameters
            """

            def __init__(self):
                pass

            @staticmethod
            def portovm(group, old_itype, new_itype):
                pvmt = (SETTINGS.constants.portovm.itype, 'psi')

                if (old_itype not in pvmt) and (new_itype not in pvmt):
                    return
                elif (old_itype in pvmt) and (new_itype in pvmt):
                    return
                elif (old_itype in pvmt) and (new_itype not in pvmt):
                    group.card.tags.itype = old_itype
                    for host in group.getHosts():
                        TGroupTriggers.TOnRemoveHostsTriggers.TAutoTriggers.portovm(group, host)
                    group.card.tags.itype = new_itype
                elif (old_itype not in pvmt) and (new_itype in pvmt):
                    for host in group.getHosts():
                        TGroupTriggers.TOnAddHostsTriggers.TAutoTriggers.portovm(group, host)
                else:
                    raise Exception("OOPS")

        @staticmethod
        def run(group, old_itype, new_itype):
            """
                Selector function. Select correct triggers to process
            """

            TGroupTriggers.TOnSetItypeTriggers.TAutoTriggers.portovm(group, old_itype, new_itype)

    class TOnUpdateGroupTriggers(object):
        """Trigger, called when group is changed"""

        class TAutoTriggers(object):
            def __init__(self):
                pass

            @staticmethod
            def default(group, old_name):
                for other_group in group.parent.get_groups():
                    if other_group.card.properties.nidx_for_group == old_name:
                        other_group.card.properties.nidx_for_group = group.card.name
                        other_group.mark_as_modified()
                    if other_group.card.properties.created_from_portovm_group == old_name:
                        other_group.card.properties.reated_from_portovm_group = group.card.name
                        other_group.mark_as_modified()

        @staticmethod
        def run(group):
            # recalculate ssd/hdd size
            from gaux.aux_volumes import get_ssd_hdd_guarantee
            need_ssd, need_ssd_str, need_hdd, need_hdd_str = get_ssd_hdd_guarantee(group)
            if need_ssd > group.card.reqs.instances.ssd.value:
                group.card.reqs.instances.ssd = ByteSize(need_ssd_str)
            if need_hdd > group.card.reqs.instances.disk.value:
                group.card.reqs.instances.disk = ByteSize(need_hdd_str)

            # set reqs.shards.min_power
            instances_power = {x.power for x in group.get_instances()}
            if instances_power and (max(instances_power) - min(instances_power) < 2):
                group.card.reqs.shards.min_power = int(max(instances_power))

    class TOnRenameGroupTriggers(object):
        """
            Trigger, called every time group is renamed
        """

        def __init__(self):
            pass

        class TManualTriggers(object):
            """
                Manual triggers (user can specify on of them in group card)
            """

            def __init__(self):
                pass

            @staticmethod
            def default(group, old_name):
                pass

            @staticmethod
            def disabled(group, old_name):
                """
                    Some groups can not be renamed (e. g. MSK_RESERVED/SAS_RESERVED/MAN_RESERVED)
                """
                raise Exception, "Group <%s> can not be renamed" % old_name


        class TAutoTriggers(object):
            def __init__(self):
                pass

            @staticmethod
            def default(group, old_name):
                for other_group in group.parent.get_groups():
                    if other_group.card.properties.nidx_for_group == old_name:
                        other_group.card.properties.nidx_for_group = group.card.name
                        other_group.mark_as_modified()
                    if other_group.card.properties.created_from_portovm_group == old_name:
                        other_group.card.properties.reated_from_portovm_group = group.card.name
                        other_group.mark_as_modified()

        @staticmethod
        def run(group, old_name):
            """
                Selector function. Select correct triggers to process
            """
            # run manual triggers
            func = getattr(TGroupTriggers.TOnRenameGroupTriggers.TManualTriggers, group.card.triggers.on_rename_group.method, None)
            if func is None:
                raise Exception("OnRenameGroup trigger <%s> not found (group %s, host %s)" % (group.card.triggers.on_rename_group.method, group.card.name, host.name))
            func(group, old_name)

            # run auto triggers
            TGroupTriggers.TOnRenameGroupTriggers.TAutoTriggers.default(group, old_name)
