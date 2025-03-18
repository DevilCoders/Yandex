from collections import OrderedDict
import copy

from core.hosts import Host, get_host_fields_info

from diffbuilder_types import mkindent, get_field_diff, NoneSomething, EDiffTypes, TDiffEntry


class THwDiffEntry(TDiffEntry):
    def __init__(self, diff_type, status, watchers, tasks, generated_diff, props=None):
        super(THwDiffEntry, self).__init__(diff_type, status, watchers, tasks, [], generated_diff, props=props)

    def report_text(self):
        copyd = copy.deepcopy(self.generated_diff)
        pretty_name = "Host %s" % (copyd["hostname"])
        copyd.pop("hostname")

        d = {
            pretty_name: copyd,
        }

        return self.recurse_report_text(d)


class HwDiffer(object):
    def __init__(self, olddb, newdb):
        self.olddb = olddb
        self.newdb = newdb

        self.host_fields = filter(lambda x: x not in ['storages', 'lastupdate'], map(lambda x: x.name, get_host_fields_info()))
        self.none_host = NoneSomething(map(lambda x: (x.name, None), get_host_fields_info()))

    def _get_host_diff(self, oldhost, newhost):
        props_diff = OrderedDict()

        if isinstance(oldhost, NoneSomething):
            status = "ADDED"
            hostname = newhost.name
        elif isinstance(newhost, NoneSomething):
            status = "REMOVED"
            hostname = oldhost.name
        else:
            status = "CHANGED"
            hostname = newhost.name

        if status in ["ADDED", "CHANGED"]:
            for field_name in self.host_fields:
                if field_name in ['lastupdate', 'change_time']:
                    continue
                get_field_diff(props_diff, field_name, getattr(oldhost, field_name), getattr(newhost, field_name))

        if status != "CHANGED" or len(props_diff) > 0:  # found something
            fields_dict = OrderedDict()
            fields_dict["status"] = status
            fields_dict["hostname"] = hostname

            fields_dict.update(props_diff)

            return fields_dict
        else:
            return None

    def _hosts_by_invnum(self, db):
        result = dict()
        for host in db.hosts.get_all_hosts():
            if host.invnum != "unknown" and host.invnum != "":
                result[host.invnum] = host
        return result

    def get_diff(self):
        modified = []
        renamed = []
        added = []
        removed = []

        oldhosts_by_invnum = self._hosts_by_invnum(self.olddb)
        newhosts_by_invnum = self._hosts_by_invnum(self.newdb)

        for host in self.newdb.hosts.get_all_hosts():
            if host.is_vm_guest():  # do not care about fake hosts
                continue

            if self.olddb.hosts.has_host(host.name):
                modified.append(host.name)
            elif host.invnum in oldhosts_by_invnum:
                renamed.append((oldhosts_by_invnum[host.invnum].name, host.name))
            else:
                added.append(host.name)
        for host in self.olddb.hosts.get_all_hosts():
            if host.is_vm_guest():  # do not care about fake hosts
                continue

            if not self.newdb.hosts.has_host(host.name) and \
                    not host.invnum in newhosts_by_invnum:
                removed.append(host.name)

        result = []
        for hostname in modified:
            result.append(self._get_host_diff(self.olddb.hosts.get_host_by_name(hostname),
                                              self.newdb.hosts.get_host_by_name(hostname)))
        for oldname, newname in renamed:
            result.append(self._get_host_diff(self.olddb.hosts.get_host_by_name(oldname),
                                              self.newdb.hosts.get_host_by_name(newname)))
        for hostname in added:
            result.append(self._get_host_diff(self.none_host, self.newdb.hosts.get_host_by_name(hostname)))
        for hostname in removed:
            result.append(self._get_host_diff(self.olddb.hosts.get_host_by_name(hostname), self.none_host))
        result = filter(lambda x: x is not None, result)

        return map(lambda x: THwDiffEntry(EDiffTypes.HW_DIFF, True, [], [], x), result)
