
from collections import namedtuple
from typing import Iterable, Tuple, Optional, List

from yc_issue_cert.config import ClusterConfig


HostGroup = namedtuple("HostGroup", ["host", "group"])


class Cluster:
    def __init__(self, name: str, config: ClusterConfig, only_hosts: Optional[List[str]],
                 exclude_hosts: Optional[List[str]]):
        self.name = name
        self.config = config
        self.only_hosts = only_hosts
        self.exclude_hosts = exclude_hosts

    @staticmethod
    def _get_base_role(base_role):
        # Allow to specify multiple sub-hosts ("services") for a single base role.
        # They're only used in filtering; secrets are uploaded with full base role name.
        if "@" in base_role:
            _, _, base_role = base_role.partition("@")
        return base_role

    def iter_hosts(self, base_roles=None) -> Iterable[HostGroup]:
        for base_role, hosts in self.config.hosts.items():
            if base_roles is not None and base_role not in base_roles:
                continue

            for host in hosts:
                if self.only_hosts and host not in self.only_hosts:
                    continue
                elif self.exclude_hosts and host in self.exclude_hosts:
                    continue
                yield HostGroup(host, Cluster._get_base_role(base_role))

    def iter_clients(self, cg_name: str) -> Iterable[HostGroup]:
        client_group = self.config.clients[cg_name]
        for sub_group, hosts in client_group.items():
            for host in hosts:
                yield HostGroup(host, sub_group)

    @property
    def secret_name_prefix(self):
        if self.config.prefix:
            return self.config.prefix

        name = self.name
        if self.config.secret_profile in ("prod", "pre-prod"):
            name = ""

        return "_".join(filter(bool, (name, self.config.scope)))

    def iter_ss_hostgroups(self, base_roles) -> Iterable[Tuple[str, str]]:
        stands = [self.config.bootstrap_stand]
        if self.config.is_cloudvm:
            base_roles = ["cloudvm"]

        for stand_name in stands:
            for base_role in base_roles:
                base_role = Cluster._get_base_role(base_role)
                yield "bootstrap_base-role_{}_{}".format(stand_name, base_role), base_role
