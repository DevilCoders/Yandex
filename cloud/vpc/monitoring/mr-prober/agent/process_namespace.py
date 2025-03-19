import ctypes
import ctypes.util
import ipaddress
import logging
import os
import pathlib
import sys
import tempfile
from typing import Optional, Dict, List, Tuple

import netifaces

if sys.platform == "linux":
    import unshare


__all__ = ["MountNamespace", "NamespaceManager"]


class MountNamespace:
    """
    Class for creating mount namespace (via unshare — see https://man7.org/linux/man-pages/man2/unshare.2.html).
    After creating the namespace we can replace some files or folders strictly for our process only.
    It's done via mounting these files via mount (https://man7.org/linux/man-pages/man2/mount.2.html) with MS_BIND
    option.

    We use this technique for overriding /etc/resolv.conf inside the prober.

    Supports only Linux, because uses Linux syscalls.
    """

    MS_BIND = 4096  # from linux/fs.h

    _libc = ctypes.CDLL(ctypes.util.find_library("c"), use_errno=True)
    _libc.mount.argtypes = (ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_ulong, ctypes.c_char_p)

    def __init__(self):
        self._postponed_mounts: List[Tuple[pathlib.Path, pathlib.Path]] = []

    def postpone_mounting(self, source: pathlib.Path, target: pathlib.Path):
        """
        Remember that some file (or folder) is needed to override inside of mount namespace when our namespace
        will be created (see self.enter()).
        """
        self._postponed_mounts.append((source, target))

    def enter(self):
        """
        Create the mount namespace and override files/folders remembered early.
        """
        # Create a new mount namespace
        unshare.unshare(unshare.CLONE_NEWNS)

        # Mount all requested paths with "-o bind":
        # https://distracted-it.blogspot.com/2017/03/installer-or-command-that-hangs-use.html
        for source, target in self._postponed_mounts:
            self._mount(source.as_posix(), target.as_posix(), "none", self.MS_BIND)

    def _mount(self, source: str, target: str, fs: str, flags: int, options: str = ""):
        ret = self._libc.mount(source.encode(), target.encode(), fs.encode(), flags, options.encode())
        if ret < 0:
            errno = ctypes.get_errno()
            raise OSError(
                errno, f"Error mounting {source} ({fs}) on {target} with options '{options}': {os.strerror(errno)}"
            )


class NamespaceManager:
    RESOLV_CONF_PATH = pathlib.Path("/etc/resolv.conf")

    _resolv_confs: Dict[str, pathlib.Path] = {}

    def find_namespace_by_prober_config(self, config: "ProberConfig") -> Optional[MountNamespace]:
        """
        Creates mount namespace for specified prober config.
        We use it for overriding /etc/resolv.conf for the prober.

        Returns None, if there is no need in namespace for this config.
        Also returns None is current platform is not Linux of if we can't retrieve DNS server address for the interface.
        """

        # # TODO: REMOVE THIS LINE. DEBUG ONLY
        # config.dns_resolving_interface = "eth0"

        # For now, we create namespace for prober's process iff config.dns_resolving_interface is specified
        if config.dns_resolving_interface is None:
            return None

        if sys.platform != "linux":
            logging.warning(
                f"Setting custom DNS resolver isn't supported on {sys.platform}. "
                "Only Linux is supported, because we use mount namespaces for this purpose."
            )
            return None

        if config.dns_resolving_interface in self._resolv_confs:
            desired_resolv_conf = self._resolv_confs[config.dns_resolving_interface]
        else:
            dns_server = self._get_dns_server_for_interface(config.dns_resolving_interface)
            # If there is no IPv4 address for the interface, let's try to get IPv6 address
            if dns_server is None:
                dns_server = self._get_dns_server_for_interface(config.dns_resolving_interface, netifaces.AF_INET6)
            if dns_server is None:
                logging.warning(f"Can't find DNS server for interface {config.dns_resolving_interface}")
                return None

            # Save desired resolv.conf to the temporary file: later we will mount it as /etc/resolv.conf inside the
            # mount namespace.
            with tempfile.NamedTemporaryFile(
                "w", prefix=f"mr-prober-resolv-conf-{config.dns_resolving_interface}-",
                delete=False
            ) as resolv_conf:
                resolv_conf.write(f"nameserver {dns_server}")
                desired_resolv_conf = pathlib.Path(resolv_conf.name)

            self._resolv_confs[config.dns_resolving_interface] = desired_resolv_conf

        namespace = MountNamespace()
        namespace.postpone_mounting(desired_resolv_conf, self.RESOLV_CONF_PATH)

        return namespace

    @staticmethod
    def _get_dns_server_for_interface(interface_name: str, proto: int = netifaces.AF_INET) -> Optional[str]:
        """
        There is no good way to get a DNS server for the specific network interface.
        Here, we implement Yandex Cloud-only solution: we know that overlay DNS server is the second address
        of the network.

        Other possible ways are to run DHCP manually, look at "resolvectl status <interface-name>" etc. We don't
        implement them now.

        Returns None if there is no non-link-local address on the interface.
        """
        addresses = netifaces.ifaddresses(interface_name)[proto]
        for address in addresses:
            try:
                network = ipaddress.ip_network(address["addr"] + "/" + address["netmask"], strict=False)
            except ValueError:
                continue

            # Ignore link-local networks
            if network.is_link_local:
                continue

            network_hosts = network.hosts()
            # Skip first address from the network's hosts
            next(network_hosts)
            # Return second address from the network's hosts
            return next(network_hosts).compressed

        return None




