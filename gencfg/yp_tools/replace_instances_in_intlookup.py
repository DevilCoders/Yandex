# coding: utf-8
import core.db


def _instances_of_intlookup(il):
    for ms in il.get_multishards():
        for br in ms.brigades:
            for base in br.basesearchers[0]:
                yield (base.host.name, base.port)


def _read_inst(fn):
    for line in open(fn):
        host, port = line.split()[0].split(':')
        yield host, int(port)


def replace(intlookup_name, instances, power=40.0):
    """
    instances: Iterable[(str, int)]
    """
    db = core.db.CURDB
    ti = db.intlookups.get_intlookup(intlookup_name)
    insts = set(instances)
    stays = set(_instances_of_intlookup(ti)).intersection(insts)
    replacement = list(insts.difference(stays))
    for ms in ti.get_multishards():
        for br in ms.brigades:
            new_basesearchers = []
            for base in br.basesearchers[0]:
                if (base.host.name, base.port) in stays:
                    new_basesearchers.append(base)
                else:
                    host, port = replacement.pop()
                    instance = core.instances.Instance(db.hosts.get_host_by_name(host), power, port, intlookup_name, 0)
                    new_basesearchers.append(instance)
            br.basesearchers[0] = new_basesearchers

    assert not replacement, 'Replacement not exhausted'
    ti.write_intlookup_to_file_json(intlookup_name + '_replaced')
