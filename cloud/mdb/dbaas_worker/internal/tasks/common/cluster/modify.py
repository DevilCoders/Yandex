"""
Common cluster modify executor.
"""

from copy import deepcopy

from ...utils import build_host_group
from ..modify import BaseModifyExecutor


class ClusterModifyExecutor(BaseModifyExecutor):
    """
    Generic class for cluster modify executors
    """

    def _modify_hosts(
        self,
        properties,
        hosts=None,
        order=None,
        restart=None,
        metadata=None,
        health_check_each=None,
        allow_fail_hosts=0,
        reverse_order=None,
        run_highstate=None,
        do_before_recreate=None,
        do_after_recreate=None,
        default_pillar=None,
    ):
        """
        Generic method for cluster modification.

        If hosts or restart are not provided, they will be obtained from task arguments.
        """
        if restart is None:
            restart = self.args.get('restart')

        if reverse_order is None:
            reverse_order = self.args.get('reverse_order')

        hosts = hosts or deepcopy(self.args['hosts'])
        for host in hosts:
            pillar = {} if default_pillar is None else deepcopy(default_pillar)
            if restart:
                pillar['service-restart'] = True
            if self.args.get('sox-changed'):
                pillar['users-modify'] = True
            if pillar:
                hosts[host]['deploy'] = {'pillar': pillar}

        if run_highstate is None:
            run_highstate = self.args.get('run-highstate')
        if run_highstate:
            for host in hosts:
                if 'pillar' in hosts[host].get('deploy', {}):
                    hosts[host]['deploy']['pillar'].update({'run-highstate': True})
                else:
                    hosts[host]['deploy'] = {'pillar': {'run-highstate': True}}
        else:
            if metadata is None:
                metadata = self.args.get('include-metadata')
            if metadata:
                for host in hosts:
                    if 'pillar' in hosts[host].get('deploy', {}):
                        hosts[host]['deploy']['pillar'].update({'include-metadata': True})
                    else:
                        hosts[host]['deploy'] = {'pillar': {'include-metadata': True}}

        host_group = build_host_group(properties, hosts)

        if reverse_order:
            self._run_operation_host_group(host_group, 'service', order=order)

        self._change_host_group(
            host_group,
            order=order,
            health_check=health_check_each,
            allow_fail_hosts=allow_fail_hosts,
            do_before_recreate=do_before_recreate,
            do_after_recreate=do_after_recreate,
        )

        if not reverse_order:
            self._run_operation_host_group(host_group, 'service', order=order)

        if restart:
            self._health_host_group(host_group)
