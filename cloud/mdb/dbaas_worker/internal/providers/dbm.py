"""
DBM interaction module
"""

import logging
import time
from copy import deepcopy
from typing import Any, Callable, Dict, Generator, Optional

from dbaas_common import retry, tracing
from paramiko.ssh_exception import SSHException

from ..exceptions import ExposedException
from .common import Change
from .deploy import DEPLOY_VERSION_V2, Jid
from .http import HTTPClient, HTTPErrorHandler
from .ssh import SSHClient


class DBMChange:
    """
    Wrapper around dbm change
    """

    jid: Optional[Jid]

    def __init__(
        self,
        operation_id: Optional[str],
        deploy: Optional[Dict[str, Any]],
        transfer: Optional[Dict[str, Any]],
        rollback_fun: Callable[[Any, Any], None] = None,
        rollback_desc: Dict[str, Any] = None,
    ) -> None:
        self.operation_id = operation_id
        self.jid = _make_jid(deploy) if deploy is not None else None
        logger = logging.getLogger(__name__)
        logger.debug('Made jid %s from deploy %s', repr(self.jid), deploy)
        self.transfer = transfer
        if operation_id and (deploy or transfer):
            raise DBMError(
                f'Got operation {operation_id} and non-operation result: ' f'deploy and transfer: {deploy} {transfer}'
            )
        if deploy and transfer:
            raise DBMError(f'Got both deploy and transfer: {deploy} {transfer}')
        if bool(rollback_desc) != bool(rollback_fun):
            raise DBMError(f'Rollback function and description should be both set: {rollback_fun} {rollback_desc}')
        self.rollback_fun = rollback_fun
        self.rollback_desc = rollback_desc

    def to_context(self):
        """
        Convert to context representation
        """
        ret = {}
        if self.operation_id is not None:
            ret['operation_id'] = self.operation_id
        elif self.jid is not None:
            ret['deploy'] = {
                'deploy_id': self.jid.jid,
                'deploy_version': self.jid.deploy_version,
                'host': self.jid.fqdn,
            }
        elif self.transfer is not None:
            ret['transfer'] = self.transfer
        if self.rollback_desc is not None:
            ret['rollback'] = self.rollback_desc

        return ret

    def __str__(self):
        jid = self.jid
        return f'{self.__class__.__name__}: {jid=}'

    def __bool__(self):
        return bool(self.operation_id) or bool(self.jid) or bool(self.transfer)


def _make_jid(deploy: Dict[str, Any]) -> Jid:
    """
    Create internal Jid from dbm output
    """
    jid = deploy['deploy_id']
    fqdn = deploy['host']
    if deploy.get('deploy_version', 2) == 2:
        deploy_version = DEPLOY_VERSION_V2
    else:
        raise RuntimeError(f'Unexpected deploy api version in deploy data: {deploy}')
    return Jid(jid=jid, fqdn=fqdn, deploy_version=deploy_version, title='dbm')


class DBMError(ExposedException):
    """
    Base DBM error
    """


class ConfigurationError(Exception):
    """
    Container/volume configuration error
    """


class DBMApi(HTTPClient):
    """
    DBM provider
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.ssh = SSHClient(config, task, queue)
        headers = {
            'Authorization': f'OAuth {self.config.dbm.token}',
        }
        self._init_session(
            self.config.dbm.url,
            default_headers=headers,
            error_handler=HTTPErrorHandler(DBMError),
            verify=self.config.main.ca_path,
        )

    def get_container(self, fqdn):
        """
        Return container options if it exists
        """
        res = self._make_request(f'api/v2/containers/{fqdn}', expect=[200, 404])
        if res.get('error'):
            return {}
        return res

    def get_volumes(self, fqdn):
        """
        Return properly formatted volumes for container
        """
        volumes = {}
        res = self._make_request(f'api/v2/volumes/{fqdn}')
        for volume in res:
            volumes[volume['path']] = volume

        return volumes

    def wait_operation(self, operation_id, timeout=600, stop_time=None):
        """
        Wait for operation completion (raise on operation error)
        """
        if operation_id is None:
            return

        if stop_time is None:
            stop_time = time.time() + timeout
        with self.interruptable:
            while time.time() < stop_time:
                operation = self._get_operation(operation_id)
                if not operation['done']:
                    self.logger.info('Waiting for dbm operation %s', operation_id)
                    time.sleep(1)
                    continue
                if not operation['error']:
                    return
                raise DBMError(operation['error'])

            msg = f'{timeout}s passed. DBM operation {operation_id} is still running'
            raise DBMError(msg)

    def _get_operation(self, operation_id):
        """
        Get operation from DBM
        """
        tracing.set_tag('dbm.operation.id', operation_id)
        return self._make_request(f'api/v2/operations/{operation_id}')

    def _delete_container(self, fqdn):
        """
        Delete existing container and return resulting deploy
        """
        paths = ','.join(sorted(self.get_volumes(fqdn).keys()))
        res = self._make_request(f'api/v2/containers/{fqdn}?save_paths={paths}', method='delete')
        return self._dbm_change_from_response(res)

    def _get_changes(self, fqdn, container, options, volumes, generation):
        """
        Get difference between target state and dbm
        """
        resources_changed = False
        volumes_changed = []

        if 'restore' in options:
            return resources_changed, volumes_changed

        for option, value in options.items():
            if container.get(option) != value:
                resources_changed = True
                break

        if container['generation'] != generation:
            resources_changed = True

        dbm_volumes = self.get_volumes(fqdn)
        for volume, volume_opts in volumes.items():
            if volume not in dbm_volumes:
                raise ConfigurationError(f'Volume {volume} not found in running container {fqdn}')
            for key, value in volume_opts.items():
                if key != 'dom0_path' and dbm_volumes[volume].get(key) != value:
                    volumes_changed.append(volume)
                    break

        return resources_changed, volumes_changed

    def _create_container(self, fqdn, data, revertable) -> DBMChange:
        """
        Create container in dbm
        """
        res = self._make_request(f'api/v2/containers/{fqdn}', method='put', data=data)
        if revertable:
            rollback_desc: Optional[Dict[str, Any]] = {'operation': 'delete', 'target': fqdn}
            rollback_fun = self._rollback_fun_from_desc(rollback_desc)
        else:
            rollback_desc = None
            rollback_fun = None
        return self._dbm_change_from_response(res, rollback_fun, rollback_desc)

    def _get_transfer(self, transfer_id):
        """
        Get transfer by id
        """
        return self._make_request(f'api/v2/transfers/{transfer_id}')

    def get_container_transfer(self, fqdn):
        """
        Get transfer by container fqdn
        """
        return self._make_request(f'api/v2/transfers/?fqdn={fqdn}')

    def _cancel_transfer(self, transfer_id):
        """
        Cancel transfer by id
        """
        return self._make_request(f'api/v2/transfers/{transfer_id}/cancel', method='post')

    def _dbm_change_from_response(self, res, rollback_fun=None, rollback_desc=None) -> DBMChange:
        """
        Build DBMChange from dbm response
        """
        return DBMChange(
            res.get('operation_id'),
            res.get('deploy'),
            self._get_transfer(res['transfer']) if 'transfer' in res else None,
            rollback_fun,
            rollback_desc,
        )

    def find_containers(self, query: dict) -> Generator[dict, None, None]:
        """
        This generator returns all containers matching query
        """
        for item in self.paginate('api/v2/containers/', query):
            yield item

    def finish_transfer(self, transfer_id) -> dict:
        """
        Finish existing transfer
        """
        return self._make_request(
            f'api/v2/transfers/{transfer_id}/finish',
            method='POST',
        )

    def update_container(self, fqdn, data, generation=None) -> DBMChange:
        """
        Modify container resources in dbm
        """
        data = deepcopy(data)
        if generation is not None:
            data['generation'] = generation
        res = self._make_request(f'api/v2/containers/{fqdn}', method='post', data=data)
        return self._dbm_change_from_response(res)

    def _modify_volume(self, fqdn, volume, data, deploy=True):
        """
        Modify volume in dbm
        """
        url_deploy = '&init_deploy=false' if not deploy else ''
        res = self._make_request(
            f'api/v2/volumes/{fqdn}?path={volume}{url_deploy}',
            method='post',
            data=data,
        )
        if not deploy:
            if 'transfer' in res:
                self.logger.info(
                    'Cancelling transfer caused by %s resize on %s due to more changes required',
                    volume,
                    fqdn,
                )
                self._cancel_transfer(res['transfer'])
            return
        return self._dbm_change_from_response(res)

    @retry.on_exception(DBMError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('DBM Container Exists')
    def _container_exists(
        self,
        cid,
        fqdn,
        geo,
        options,
        volumes,
        bootstrap_cmd,
        generation,
        secrets,
        revertable,
    ) -> DBMChange:
        # pylint: disable=too-many-arguments,too-many-locals
        tracing.set_tag('cluster.id', cid)
        tracing.set_tag('dbm.container.fqdn', fqdn)

        container = self.get_container(fqdn)
        if not container:
            change = self._create_container(
                fqdn,
                dict(
                    **options,
                    volumes=[dict(**value, path=key) for key, value in volumes.items()],
                    fqdn=fqdn,
                    geo=geo,
                    cluster=cid,
                    project=self.config.dbm.project,
                    bootstrap_cmd=bootstrap_cmd,
                    generation=generation,
                    secrets=(secrets or {}),
                ),
                revertable,
            )
            return change

        resources_changed, volumes_changed = self._get_changes(fqdn, container, options, volumes, generation)
        transfer = self.get_container_transfer(fqdn)
        if (resources_changed or volumes_changed) and transfer:
            self.logger.info('Cancelling existing transfer on %s due to changes', fqdn)
            self._cancel_transfer(transfer['id'])
            transfer = None
        for volume in volumes_changed[:-1]:
            self._modify_volume(fqdn, volume, volumes[volume], deploy=False)

        if volumes_changed:
            if resources_changed:
                self._modify_volume(fqdn, volumes_changed[-1], volumes[volumes_changed[-1]], deploy=False)
            else:
                change = self._modify_volume(fqdn, volumes_changed[-1], volumes[volumes_changed[-1]])
                return change

        if resources_changed:
            change = self.update_container(fqdn, options, generation)
            return change

        if transfer:
            transfer['continue'] = True
        change = DBMChange(None, None, transfer)
        return change

    def container_exists(
        self,
        cid,
        fqdn,
        geo,
        options,
        volumes,
        bootstrap_cmd,
        platform_id,
        secrets=None,
        revertable=False,
    ) -> DBMChange:
        """
        Create new or modify existing container in dbm
        """
        change_from_context = self.context_get(f'{fqdn}.exists')
        if change_from_context:
            if change_from_context.get('rollback'):
                rollback = self._rollback_fun_from_desc(change_from_context.get('rollback'))
            else:
                rollback = None
            self.add_change(Change(f'container.{fqdn}', 'exists', rollback=rollback))
            return DBMChange(
                change_from_context.get('operation_id'),
                change_from_context.get('deploy'),
                change_from_context.get('transfer'),
            )
        change = self._container_exists(
            cid,
            fqdn,
            geo,
            options,
            volumes,
            bootstrap_cmd,
            int(platform_id),
            secrets,
            revertable,
        )
        self.add_change(
            Change(
                f'container.{fqdn}',
                'exists',
                context={f'{fqdn}.exists': change.to_context()} if change else {},
                rollback=change.rollback_fun if change else None,
            ),
        )
        return change

    @retry.on_exception((DBMError, SSHException), factor=10, max_wait=60, max_tries=6)
    @tracing.trace('DBM Container Absent')
    def _container_absent(self, fqdn):
        tracing.set_tag('dbm.container.fqdn', fqdn)
        if self.get_container(fqdn):
            transfer = self.get_container_transfer(fqdn)
            if transfer:
                with self.ssh.get_conn(transfer['src_dom0'], use_agent=True) as conn:
                    self.ssh.exec_command(
                        transfer['src_dom0'],
                        conn,
                        f'/usr/sbin/move_container.py --revert {transfer["container"]}',
                        interruptable=True,
                    )
            return self._delete_container(fqdn)

    def list_volumes(self) -> Generator[dict, None, None]:
        """
        List all volumes
        """
        for item in self.paginate('api/v2/volumes/', {}):
            yield item

    def paginate(self, url_path: str, base_query: dict) -> Generator[dict, None, None]:
        """
        DBM pagination helper
        """
        offset = 0
        limit = 1000
        while True:
            query = deepcopy(base_query)
            query.update(offset=offset, limit=limit)
            search_string = ';'.join([f'{key}={value}' for key, value in query.items()])
            data = self._make_request(url_path, params={'query': search_string})
            for result in data:
                yield result
            if len(data) < limit:
                break
            offset += limit

    def container_absent(self, fqdn):
        """
        Delete container if exists
        """
        change_from_context = self.context_get(f'{fqdn}.delete')
        if change_from_context:
            self.add_change(Change(f'container.{fqdn}', 'absent'))
            return DBMChange(
                change_from_context.get('operation_id'),
                change_from_context.get('deploy'),
                change_from_context.get('transfer'),
            )
        change = self._container_absent(fqdn)
        self.add_change(
            Change(
                f'container.{fqdn}',
                'absent',
                context={f'{fqdn}.delete': change.to_context()} if change is not None else {},
            ),
        )
        return change

    def _rollback_fun_from_desc(self, rollback_desc):
        """
        Build rollback function from description
        """
        operation = rollback_desc.get('operation')
        if operation == 'delete':
            target = rollback_desc.get('target')
            if not target:
                raise DBMError(f'Got delete operation without target: {rollback_desc}')
            return lambda task, safe_revision: self._container_absent(target)
        raise DBMError(f'Unexpected rollback description: {rollback_desc}')
