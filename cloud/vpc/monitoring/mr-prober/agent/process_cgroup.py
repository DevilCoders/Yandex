import ipaddress
import logging
import pathlib
import subprocess
from typing import Optional, Dict, List

import netifaces

import settings

__all__ = ["Cgroup", "RoutingTableManager", "CgroupManager"]

# just a random 16-bit value which we use as a major handle number of net_cls.classid
# (see https://www.kernel.org/doc/Documentation/cgroup-v1/net_cls.txt)
NET_CLS_MAJOR_HANDLE_NUMBER = 0xD00D


class RoutingTableManager:
    ROUTING_TABLES_FILE = pathlib.Path("/etc/iproute2/rt_tables")

    # Static field for already configured routing tables
    _configured_routing_tables = {}

    def get_or_create_routing_table(self, routing_table_name: str, default_routing_interface: str) -> int:
        """
        Returns index of routing table in /etc/iproute2/rt_tables
        """
        if routing_table_name in RoutingTableManager._configured_routing_tables:
            # Don't repeat ourselves and don't configure routing table twice in one session.
            # But if agent is restarting, it may re-configure routing table. It's okay, because
            # configuring is enough idempotent.
            return RoutingTableManager._configured_routing_tables[routing_table_name]

        routing_tables = self._read_routing_tables()

        if routing_table_name in routing_tables:
            routing_table_index = routing_tables[routing_table_name]
        else:
            # Allocate new routing table and append its index into /etc/iproute2/rt_tables
            routing_table_index = self._find_free_routing_table_index()
            logging.info(f"Creating new routing table {routing_table_name} with index {routing_table_index}. "
                         f"Save it to {self.ROUTING_TABLES_FILE.as_posix()}")
            with self.ROUTING_TABLES_FILE.open("a") as routing_tables_file:
                routing_tables_file.write(f"\n{routing_table_index} {routing_table_name}\n")

        self._configure_routing_table(routing_table_index, routing_table_name, default_routing_interface)
        RoutingTableManager._configured_routing_tables[routing_table_name] = routing_table_index

        return routing_table_index

    def _read_routing_tables(self) -> Dict[str, int]:
        result = {}
        for line in self.ROUTING_TABLES_FILE.read_text().splitlines():
            line = line.strip()
            if line.startswith("#") or not line:
                # Ignore comments and empty lines
                continue
            try:
                table_id, table_name = line.split(None, 2)
                result[table_name] = int(table_id)
            except ValueError:
                # Just in case if /etc/iproute2/rt_tables has invalid format
                pass
        return result

    def _find_free_routing_table_index(self):
        routing_table_indices = self._read_routing_tables().values()
        # Routing table index should be less than 253 to have larger priority than "main" and "default" tables
        # (see https://timeweb.com/ru/community/articles/nastroyka-neskolkih-tablic-marshrutizacii-na-odnom-servere)
        for index in range(2, 253):
            if index not in routing_table_indices:
                return index
        raise ValueError(
            "Can't find non-used routing table index: "
            f"all values in [2..252] already defined in {self.ROUTING_TABLES_FILE.as_posix()}"
        )

    def _configure_routing_table(self, table_index: int, table_name: str, default_routing_interface: str):
        """
        Configures routing table: adds ip rule and ip route.
        See https://www.evolware.org/?p=369 for details.
        """
        # First, create ip rule, which says that all marked (by special fwmark) packets should follow our routing table
        self._remove_ip_rules_in_table(table_name)
        self._add_ip_rule_in_table(table_index, table_name)

        # Second, add ip routes, which route all IPv4 and IPv6 traffic via specified interface
        self._add_default_routes_to_table(default_routing_interface, table_name)

    @staticmethod
    def _remove_ip_rules_in_table(table_name: str):
        """
        There is no way to remove all rules for table in one command, so we have to loop over rules and
        remove them one by one.
        See https://stackoverflow.com/a/29688220/1494610
        """
        for ip_version in ("4", "6"):
            script = f"while ip -{ip_version} rule delete from 0/0 to 0/0 table {table_name}; do true; done"
            _run_process_and_log_output(["/bin/bash", "-c", script])

    @staticmethod
    def _add_ip_rule_in_table(table_index: int, table_name: str):
        for ip_version in ("4", "6"):
            ip_rule_command = [
                settings.IP_TOOL_PATH, "-" + str(ip_version), "rule", "add", "fwmark", str(table_index),
                "table", table_name
            ]
            logging.info(
                f"Adding IPv{ip_version} rule for redirecting "
                f"fwmarked packets to the routing table: {' '.join(ip_rule_command)}"
            )
            _run_process_and_log_output(ip_rule_command, check=True)

    @staticmethod
    def _add_default_routes_to_table(default_routing_interface: str, table_name: str):
        for ip_version in ("4", "6"):
            # Delete all other routes in the table:
            ip_route_command = [
                settings.IP_TOOL_PATH, "-" + str(ip_version), "route", "delete", "default",
                "table", table_name
            ]
            _run_process_and_log_output(ip_route_command)

            # Add the only default route rule
            ip_route_command = [
                settings.IP_TOOL_PATH, "-" + str(ip_version), "route", "add", "default",
                "dev", default_routing_interface,
                "table", table_name
            ]
            logging.info(
                f"Adding default IPv{ip_version} route into the routing table {table_name}: "
                f"{' '.join(ip_route_command)}"
                )
            _run_process_and_log_output(ip_route_command, check=True)


class Cgroup:
    """
    Wrapper for working with cgroups via filesystem interface mounted to /sys/fs/cgroup.
    See https://man7.org/linux/man-pages/man7/cgroups.7.html

    Inspired by https://github.com/francisbouvier/cgroups/blob/master/cgroups/cgroup.py, but this library has bugs
    and doesn't support all hierarchies.
    """
    CGROUPS_PATH = pathlib.Path("/sys/fs/cgroup")

    def __init__(self, name: str, hierarchy: str):
        hierarchy_path = self.CGROUPS_PATH / hierarchy
        if not hierarchy_path.exists():
            raise RuntimeError(f"Cgroup hierarchy {hierarchy_path} is not mounted. Check {hierarchy_path.as_posix()}")

        cgroup_path = hierarchy_path / name
        cgroup_path.mkdir(exist_ok=True)

        self.name = name
        self._path = cgroup_path

    def get_cgroup_file(self, filename: str) -> pathlib.Path:
        return self._path / filename

    def add(self, pid: int):
        self._check_process_exists(pid)

        tasks_file = self.get_cgroup_file("tasks")
        cgroups_pids = set(line.strip() for line in tasks_file.read_text().splitlines())
        if str(pid) in cgroups_pids:
            # Process is already in cgroup
            return

        with tasks_file.open("a") as f:
            f.write(f"{pid}\n")

    @staticmethod
    def _check_process_exists(pid: int):
        process_status_file = pathlib.Path(f"/proc/{pid}/status")
        if not process_status_file.exists():
            raise RuntimeError(f"Pid {pid} does not exists")


class CgroupManager:
    # Static field for already configured cgroups
    _configured_cgroups = {}

    def __init__(self):
        self._routing_table_manager = RoutingTableManager()

    def find_cgroup_by_prober_config(self, config: "ProberConfig") -> Optional[Cgroup]:
        """
        Find (or create if it doesn't exist) cgroup for specified prober config.
        Returns None, if there is no need in cgroup for this config.
        """

        # For now, we create cgroup for prober's process iff config.default_routing_interface is specified
        if config.default_routing_interface is None:
            return None

        cgroup_and_routing_table_name = f"mr-prober-{config.default_routing_interface}"
        if cgroup_and_routing_table_name in CgroupManager._configured_cgroups:
            return CgroupManager._configured_cgroups[cgroup_and_routing_table_name]

        routing_table_index = self._routing_table_manager.get_or_create_routing_table(
            cgroup_and_routing_table_name, config.default_routing_interface
        )

        cgroup = Cgroup(cgroup_and_routing_table_name, hierarchy="net_cls")
        self._configure_cgroup(cgroup, routing_table_index, config.default_routing_interface)
        CgroupManager._configured_cgroups[cgroup_and_routing_table_name] = cgroup
        return cgroup

    def _configure_cgroup(self, cgroup: Cgroup, routing_table_index: int, interface_name: str):
        cgroup_classid = (NET_CLS_MAJOR_HANDLE_NUMBER << 16) | routing_table_index

        # We have to configure our cgroup. For network routing we define classid, and create iptables rule for marking
        # packets originated by process with this classid.
        logging.info(f"Creating cgroup {cgroup.name}")

        # Cgroup's "net_cls.classid" file should contain 32-bit hex value:
        # https://mesos.apache.org/documentation/latest/isolators/cgroups-net-cls/.
        net_cls_classid_file = cgroup.get_cgroup_file("net_cls.classid")
        net_cls_classid_file.write_text(hex(cgroup_classid))

        logging.info(f"Assigned net_cls.classid for cgroup {cgroup.name} and "
                     f"saved it to {net_cls_classid_file.as_posix()}: {cgroup_classid}")

        # Also, we should create iptables rule which will mark all packed originated in our cgroup with fwmark ...
        self._create_iptables_mark_rule(cgroup_classid, routing_table_index)
        # ... and iptables rule for SNAT-int outgoing packets with correct source address
        self._create_iptables_snat_rule(interface_name)

    @staticmethod
    def _check_and_create_iptables_rule(command: List[str], human_readable_explanation: str):
        # Replace -A to -C and check, if rule already exists?
        check_command = [cmd if cmd != "-A" else "-C" for cmd in command]
        process = _run_process_and_log_output(check_command, fails_ok=True)
        if process.returncode == 0:
            # Rule already exists, don't add it again
            return

        logging.info(f"Creating iptables rule for {human_readable_explanation}: {' '.join(command)}")

        # Potentially this call may block execution of probers, but we have to do it only once
        # after receiving prober configs, so it's acceptable, I guess
        _run_process_and_log_output(command, check=True)

    def _create_iptables_mark_rule(self, cgroup_classid: int, mark: int):
        for iptables_tool in [settings.IPTABLES_TOOL_PATH, settings.IP6TABLES_TOOL_PATH]:
            command = [iptables_tool, "-w", "-t", "mangle", "-A", "OUTPUT", "-m", "cgroup",
                       "--cgroup", hex(cgroup_classid), "-j", "MARK", "--set-mark", str(mark)]

            self._check_and_create_iptables_rule(command, "marking outgoing packets by fwmark")

    def _create_iptables_snat_rule(self, interface_name: str):
        # https://st.yandex-team.ru/CLOUD-97924. We have to add SNAT rule to POSTROUTING chain, otherwise
        # outgoing packets may have source address from another network interface
        for ip_version in [4, 6]:
            if ip_version == 4:
                iptables_tool = settings.IPTABLES_TOOL_PATH
                proto = netifaces.AF_INET
            else:
                iptables_tool = settings.IP6TABLES_TOOL_PATH
                proto = netifaces.AF_INET6

            ip_address = self._get_interface_ip_address(interface_name, proto)
            command = [iptables_tool, "-w", "-t", "nat", "-A", "POSTROUTING", "-o", interface_name,
                       "-j", "SNAT", "--to-source", ip_address]
            self._check_and_create_iptables_rule(command, "SNAT-ing outgoing packets with correct source address")

    @staticmethod
    def _get_interface_ip_address(interface_name: str, proto: int) -> str:
        """
        Returns first non-link-local address of network interface
        """
        for address in netifaces.ifaddresses(interface_name)[proto]:
            # Link local addresses may have "%" inside, skip it. I.e., fe80::d20d:13ff:fede:2242%eth0
            if "%" in address["addr"]:
                continue
            try:
                # Try to parse IP address and check if it is link local
                if ipaddress.ip_address(address["addr"]).is_link_local:
                    continue
            except ValueError:
                pass

            # Return first non-link-local address of the interface
            return address["addr"]


def _run_process_and_log_output(command: List[str], fails_ok: bool = False, **subprocess_run_kwargs) -> subprocess.CompletedProcess:
    logging.debug(f"Running \"{' '.join(command)}\"")
    subprocess_run_kwargs["capture_output"] = True
    try:
        process = subprocess.run(command, **subprocess_run_kwargs)
    except subprocess.CalledProcessError as e:
        # Catch exception if check=True passed and process terminated with non-zero exit code.
        # Do it just for correct logging.
        logging.error(f"Process exited with code {e.returncode}")
        logging.error(f"Process stdout:\n{e.stdout}")
        logging.error(f"Process stderr:\n{e.stderr}")
        raise e

    if process.returncode != 0 and not fails_ok:
        logging.warning(f"Process exited with code {process.returncode}")
    if process.stdout:
        logging.info(f"Process stdout:\n{process.stdout}")
    if process.stderr:
        logging.error(f"Process stderr:\n{process.stderr}")

    return process
