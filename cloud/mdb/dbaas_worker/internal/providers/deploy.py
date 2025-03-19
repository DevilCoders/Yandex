"""
MDB API Deploy interaction module

Code and data duplication is there to make transition to Deploy V2 easier.
Plan is to remove everything V1-related when V2 is deployed everywhere.
"""

import base64
import dataclasses
import json
import time
import urllib.parse
from operator import itemgetter
from typing import Any, Dict, List, Optional, TypedDict, Union

import opentracing

from dbaas_common import retry, tracing
from .common import Change
from .http import HTTPClient, HTTPErrorHandler
from .iam_jwt import IamJwt
from ..exceptions import ExposedException
from ..logs import get_task_prefixed_logger

DEPLOY_VERSION_V2 = 2


@dataclasses.dataclass
class DeployShipmentCommand:
    arguments: List[str]
    timeout: int
    type: str


@dataclasses.dataclass
class DeployShipmentData:
    commands: List[DeployShipmentCommand]
    fqdns: List[str]
    timeout: int
    parallel: int
    stopOnErrorCount: int


class Jid:  # pylint: disable=too-few-public-methods
    """
    Wrapper around deploy Shipment Id
    """

    def __init__(self, jid: str, fqdn: str, deploy_version: Optional[int], title: str = None) -> None:
        self.jid = jid  # ACHTUNG! In fact it is shipment_id. However for historic reasons it is named 'jid'
        self.fqdn = fqdn
        self.deploy_version = deploy_version
        self.title = title

    def __str__(self) -> str:
        return str(self.jid)

    def __repr__(self) -> str:
        version_str = 'v?'
        if self.deploy_version:
            version_str = 'v' + str(self.deploy_version)
        if self.title:
            return 'deploy{1}.{0.title}.{0.fqdn}.{0.jid}'.format(self, version_str)
        return 'deploy{1}.{0.fqdn}.{0.jid}'.format(self, version_str)


class DeployError(ExposedException):
    """
    Base deploy error
    """


class DeployErrorTimeout(DeployError):
    """
    Timeout deploy error
    """


class DeployErrorMaxAttempts(DeployError):
    """
    Max attempts reached deploy error
    """

    def __init__(self, message, shipments):
        super().__init__(message)
        self.shipments = shipments


class DeployApiError(ExposedException):
    """
    Unexpected deploy api response error
    """


class _AwaitingShipment:
    """
    Shipment that we wait for
    """

    def __init__(self, shipment: Jid, attempts: int) -> None:
        self.shipment = shipment
        self.finished = False
        self.attempts = attempts
        self.old_jids = []  # type: List[str]

    def can_restart(self) -> bool:
        """
        Check if shipment is allowed to restart
        """
        return self.attempts > 1

    def restart_jid(self, jid: str) -> None:
        """
        Save new jid and remember previous
        """
        self.old_jids.append(self.shipment.jid)
        self.shipment.jid = jid
        self.attempts = self.attempts - 1

    def __repr__(self) -> str:
        if not self.old_jids:
            return repr(self.shipment)
        suffix = f'restarted {len(self.old_jids)} times as {", ".join([str(x) for x in self.old_jids])}'
        return f'{self.shipment!r}({suffix})'


# Jobs
JobRespType = TypedDict(
    'JobRespType', {'id': str, 'extId': str, 'commandId': str, 'status': str, 'createdAt': int, 'updatedAt': int}
)
JobsListRespType = TypedDict('JobsListRespType', {'jobs': list[JobRespType]})

# JobResults
JobResultRespType = TypedDict(
    'JobResultRespType',
    {
        'id': int,
        'extId': str,
        'fqdn': str,
        'order': int,
        'status': str,
        'recordedAt': int,
        'result': Optional[Dict[str, Any]],
    },
)
JobResultsListRespType = TypedDict('JobResultsListRespType', {'jobResults': list[JobResultRespType]})


class DeployApiV2(HTTPClient):
    """
    Deploy provider
    """

    SYNCALL_TIMEOUT_RATIO = 2.0 / 15.0

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.base_url = urllib.parse.urljoin(self.config.deploy.url_v2, 'v1/')

        self.iam_jwt = None
        if not self.config.deploy.token_v2:
            self.iam_jwt = IamJwt(config, task, queue)

        self._init_session(
            self.config.deploy.url_v2,
            'v1/',
            default_headers=self._get_headers,
            error_handler=HTTPErrorHandler(DeployApiError),
            verify=self.config.deploy.ca_path or self.config.main.ca_path,
        )
        self.default_max_attempts = self.config.deploy.attempts
        self.default_timeout = self.config.deploy.timeout

    def _get_headers(self):
        if self.iam_jwt is not None:
            jwt_token = self.iam_jwt.get_token()
            return {
                'Authorization': f'Bearer {jwt_token}',
            }
        return {'Authorization': f'OAuth {self.config.deploy.token_v2}'}

    def _get_seconds_to_deadline(self) -> int:
        """
        Return seconds to deadline
        """
        return max(int(self.task['timeout']), self.default_timeout)

    def _compute_timeout(self):
        """
        Compute timeout base on deadline
        """
        return self._get_seconds_to_deadline() // self.default_max_attempts

    @tracing.trace('Deploy Shipment Poll', ignore_active_span=True)
    def _get_shipment_jobs(self, shipment_id):
        tracing.set_tag('deploy.shipment.id', shipment_id)
        params = {'shipmentId': shipment_id}
        return self._make_request('jobs', params=params)

    @tracing.trace('Deploy JobResult Poll', ignore_active_span=True)
    def _job_results(self, fqdn, ext_job_id) -> JobResultsListRespType:
        tracing.set_tag('deploy.fqdn.id', fqdn)
        tracing.set_tag('deploy.extId.id', ext_job_id)
        params = {
            'fqdn': fqdn,
            'extJobId': ext_job_id,
        }
        return self._make_request('jobresults', params=params)

    @tracing.trace('Deploy Shipment Make')
    def _make_shipment(self, shipment_data: dict) -> str:
        """
        Create new shipment from shipment_data return shipment id
        """
        tracing.set_tag('deploy.shipment.fqdns', shipment_data['fqdns'])

        # we should recalculate commands timeout and shipment timeout base on deadline
        data = {
            'commands': shipment_data['commands'],
            'fqdns': shipment_data['fqdns'],
            'parallel': shipment_data['parallel'],
            'stopOnErrorCount': shipment_data['stopOnErrorCount'],
            'timeout': shipment_data['timeout'],
        }
        return self._make_request('shipments', 'post', data=data)['id']

    def create_shipment(self, shipment_data: DeployShipmentData) -> List[Jid]:
        """
        Create new shipment public method
        """
        data_dict = dataclasses.asdict(shipment_data)
        jid = self._make_shipment(data_dict)
        result = [
            Jid(
                jid=jid,
                fqdn=fqdn,
                deploy_version=DEPLOY_VERSION_V2,
                title=str(shipment_data.commands),
            )
            for fqdn in shipment_data.fqdns
        ]
        return result

    def _restart_shipment(self, shipment_data: dict) -> str:
        return self._make_shipment(shipment_data)

    @retry.on_exception(DeployApiError, factor=1, max_wait=10, max_tries=6)
    def _get_shipment_internal(self, shipment_id):
        return self._make_request(f'shipments/{shipment_id}')

    @tracing.trace('Deploy Shipment Poll', ignore_active_span=True)
    def _get_shipment(self, shipment_id):
        tracing.set_tag('deploy.shipment.id', shipment_id)
        return self._get_shipment_internal(shipment_id)

    def wait(self, task_ids: List[Jid], max_attempts=None) -> None:
        """
        wait
        """
        self.logger.debug('Running deployV2 wait: %r', task_ids)

        deadline = time.time() + self._get_seconds_to_deadline()
        awaiting_shipments = [_AwaitingShipment(i, max_attempts or self.default_max_attempts) for i in task_ids]
        ran_out_of_attempts = False

        def get_running_shipments():
            return [s for s in awaiting_shipments if not s.finished]

        with self.interruptable:
            while time.time() < deadline:
                for wait_for in get_running_shipments():
                    shipment_data = self._get_shipment(wait_for.shipment.jid)
                    self.logger.debug('Shipment %r data is %r', wait_for, shipment_data)
                    shipment_status = shipment_data['status']

                    if shipment_status == 'inprogress':
                        continue

                    if shipment_status == 'done':
                        wait_for.finished = True
                        continue

                    if shipment_status in ('error', 'timeout'):
                        self.logger.info('Deploy job %r failed. Has %d attempts', wait_for.shipment, wait_for.attempts)
                        if wait_for.can_restart():
                            new_jid = self._restart_shipment(shipment_data)
                            self.add_change(
                                Change(
                                    repr(wait_for.shipment),
                                    'restarted',
                                    context={f'{wait_for.shipment.fqdn}.{wait_for.shipment.title}': new_jid},
                                    rollback=Change.noop_rollback,
                                )
                            )
                            wait_for.restart_jid(new_jid)
                            continue

                        ran_out_of_attempts = True
                        break

                    raise RuntimeError(
                        f'Got shipment in unexpected status: {shipment_status}, shipment: {shipment_data!r}'
                    )

                if not get_running_shipments():
                    return

                if ran_out_of_attempts:
                    break

                time.sleep(1)

            failed_shipments = []
            for shipment in get_running_shipments():
                shipment_data = self._get_shipment(wait_for.shipment.jid)
                if shipment_data['status'] in ('error', 'timeout'):
                    failed_shipments.append(shipment)

            if ran_out_of_attempts:
                raise DeployErrorMaxAttempts(
                    f'Failed deploy for {failed_shipments!r}: Max attempts reached', shipments=failed_shipments
                )
            else:
                raise DeployErrorTimeout(f'Failed deploy for {failed_shipments!r}: Timeout')

    def run(
        self,
        name: str,
        pillar: dict = None,
        deploy_title: str = None,
        method: dict = None,
        rollback=None,
        critical=False,
    ) -> Jid:
        """
        Initiate deploy on host
        """
        jid_from_context = self.context_get(f'{name}.{deploy_title}')
        if jid_from_context:
            jid = Jid(jid=jid_from_context, fqdn=name, deploy_version=DEPLOY_VERSION_V2, title=deploy_title)
            self.add_change(Change(repr(jid), 'started', rollback=rollback, critical=critical))
            return jid

        self.logger.debug('Running deployV2 run: %r', name)

        timeout = self._compute_timeout()
        syncall_timeout = int(timeout * self.SYNCALL_TIMEOUT_RATIO)

        job_pillar = {'feature_flags': self.task['feature_flags']}
        if pillar:
            job_pillar.update(pillar)
        hs_args = ['pillar={pillar}'.format(pillar=json.dumps(job_pillar)), 'queue=True']

        if method is None:
            data = {
                'commands': [
                    {
                        'type': 'saltutil.sync_all',
                        'arguments': [],
                        'timeout': syncall_timeout,
                    },
                    {
                        'type': 'state.highstate',
                        'arguments': hs_args,
                        'timeout': timeout - syncall_timeout,
                    },
                ],
                'fqdns': [name],
                'parallel': 1,
                'stopOnErrorCount': 1,
                'timeout': timeout,
            }
        else:
            data = method

        with opentracing.global_tracer().start_active_span(
            'Deploy Shipment Create',
            finish_on_close=True,
        ) as scope:
            scope.span.set_tag('deploy.shipment.fqdns', [name])
            scope.span.set_tag('deploy.minion.fqdn', name)

            res = self._make_request('shipments', 'post', data=data)

        jid = Jid(jid=res['id'], fqdn=name, deploy_version=DEPLOY_VERSION_V2, title=deploy_title)
        self.add_change(
            Change(
                repr(jid),
                'started',
                context={f'{name}.{deploy_title}': res['id']},
                rollback=rollback,
                critical=critical,
            )
        )
        return jid

    def create_minion(self, fqdn, group):
        """
        Creates minion in new deploy system
        """
        self.logger.debug('Creating minion in deployV2: %r', fqdn)
        self.add_change(
            Change(f'deploy_v2.minion.{fqdn}', 'created', rollback=lambda task, safe_revision: self.delete_minion(fqdn))
        )
        data = {
            'fqdn': fqdn,
            'group': group,
            'autoReassign': True,
        }
        self._make_request('minions/{fqdn}'.format(fqdn=fqdn), 'put', data=data)

    def create_minion_region_aware(self, fqdn, region_id):
        """
        Creates minion in new deploy system
        """

        group = self.config.deploy.regional_mapping.get(region_id, None)
        if not group:
            group = self.config.deploy.group

        self.logger.debug('Creating minion %r in region %r with target group %r', fqdn, region_id, group)
        self.add_change(
            Change(f'deploy_v2.minion.{fqdn}', 'created', rollback=lambda task, safe_revision: self.delete_minion(fqdn))
        )
        data = {
            'fqdn': fqdn,
            'group': group,
            'autoReassign': True,
        }
        self._make_request('minions/{fqdn}'.format(fqdn=fqdn), 'put', data=data)

    def delete_minion(self, fqdn):
        """
        Deletes minion from new deploy system
        """
        self.logger.debug('Deleting minion from deployV2: %r', fqdn)
        self._make_request('minions/{fqdn}'.format(fqdn=fqdn), 'delete', expect=[200, 404])

    def unregister_minion(self, fqdn):
        """
        Deletes minion key from deploy system
        """
        unregister_change = 'unregister initiated'
        unregistered = self.context_get(f'{fqdn}.unregistered')
        if unregistered:
            self.add_change(Change(fqdn, unregister_change))
            return
        self._make_request(f'minions/{fqdn}/unregister', 'post')
        self.add_change(
            Change(
                fqdn,
                unregister_change,
                context={f'{fqdn}.unregistered': True},
            )
        )

    def has_minion(self, fqdn):
        """
        Checks if deploy v2 has this minion
        """
        self.logger.debug('V2 checking if minion %r is v2 minion', fqdn)
        res = self._make_request(f'minions/{fqdn}', 'get', expect=[200, 404])
        if not res.get('fqdn'):
            return False
        return True

    def is_minion_registered(self, fqdn):
        """
        Checks if deploy v2 has this minion and minion succesfully registered on master
        """
        self.logger.debug('V2 checking if minion %r is registered', fqdn)
        res = self._make_request(f'minions/{fqdn}', 'get', expect=[200, 404])
        return bool(res.get('registered'))

    def wait_minions_registered(self, *fqdns):
        """
        Wait until minions are registered in salt master
        """
        self.logger.debug('Running deployV2 wait minions registered: %s', ', '.join(fqdns))
        deadline = time.time() + self._get_seconds_to_deadline()
        awaiting_minions = set(fqdns)

        with self.interruptable:
            while time.time() < deadline:
                for fqdn in list(awaiting_minions):
                    if self.is_minion_registered(fqdn):
                        self.logger.debug('Minion %s is registered', fqdn)
                        awaiting_minions.discard(fqdn)
                if not awaiting_minions:
                    break
                time.sleep(5)

        if awaiting_minions:
            raise DeployError(f'Minions are not registered within timeout: {", ".join(awaiting_minions)}')

    def has_job(self, jid):
        """
        Checks if deploy v2 has this job
        """
        self.logger.debug('V2 checking if jid %r is v2 shipment', jid)
        res = self._make_request(f'shipments/{jid}', 'get', expect=[200, 404])
        if res.get('message') == 'data not found':
            return False
        return True


class DeployAPI:
    """
    Abstract Deploy Provider
    """

    # pylint: disable=too-many-instance-attributes

    def __init__(self, config, task, queue):
        self._deploy_apis = {
            DEPLOY_VERSION_V2: DeployApiV2(config, task, queue),
        }
        self.new_minions_deploy_version = config.deploy.version

        if self.new_minions_deploy_version != DEPLOY_VERSION_V2:
            raise DeployError(f'Unexpected version for new minions: {self.new_minions_deploy_version}')

        if not config.deploy.group:
            raise DeployError('Deploy V2 is set for new hosts but no deploy group is set')

        self.default_max_attempts = config.deploy.attempts
        self.default_timeout = config.deploy.timeout
        self.task = task
        self.logger = get_task_prefixed_logger(task, __name__)

    def get_seconds_to_deadline(self) -> int:
        """
        Return seconds to deadline
        """
        return max(int(self.task['timeout']), self.default_timeout)

    def wait(self, task_ids: List[Jid], max_attempts=None) -> None:
        """
        Wise wait
        """

        self.logger.debug('Running deploy wait: {jids}'.format(jids=task_ids))

        if not task_ids:
            return

        versioned_jids = [[], []]  # type: List[List[Jid]]

        # Separate jid versions
        for jid in task_ids:
            # If jid has no deploy version, deduce it
            if jid.deploy_version is None:
                jid.deploy_version = self._get_deploy_version_from_jid(jid)
                self.logger.debug(
                    'Determined deploy version for jid {jid}: {version}'.format(jid=jid.jid, version=jid.deploy_version)
                )
            else:
                self.logger.debug(
                    'Using deploy version for jid {jid}: {version}'.format(jid=jid.jid, version=jid.deploy_version)
                )

            versioned_jids[jid.deploy_version - 1].append(jid)

        for deploy_version, jids in enumerate(versioned_jids, start=1):
            if not jids:
                continue

            self.logger.debug(
                'Waiting for jids {jids} of deploy version {version}'.format(jids=jids, version=deploy_version)
            )
            self._get_deploy_api(deploy_version).wait(jids, max_attempts)
            self.logger.debug(
                'Succeeded in waiting for jids {jids} of deploy version {version}'.format(
                    jids=jids, version=deploy_version
                )
            )

    def create_shipment(self, shipment_data: DeployShipmentData) -> List[Jid]:
        """
        Creates new shipment
        """
        return self._get_deploy_api(DEPLOY_VERSION_V2).create_shipment(shipment_data)

    def run(
        self,
        name: str,
        pillar: dict = None,
        deploy_title: str = None,
        method: dict = None,
        rollback=None,
        deploy_version: int = None,
        critical=False,
    ) -> Jid:
        """
        Wise run
        """

        self.logger.debug('Running deploy run: {fqdn}'.format(fqdn=name))

        if deploy_version is None:
            deploy_version = self.get_deploy_version_from_minion(name)
            self.logger.debug(
                'Determined deploy version for minion {fqdn}: {version}'.format(fqdn=name, version=deploy_version)
            )
        else:
            self.logger.debug(
                'Received deploy version from caller for minion {fqdn}: {version}'.format(
                    fqdn=name, version=deploy_version
                )
            )

        return self._get_deploy_api(deploy_version).run(name, pillar, deploy_title, method, rollback, critical=critical)

    def create_minion(self, fqdn, group):
        """
        Creates minion in new deploy system
        """

        if self.new_minions_deploy_version != DEPLOY_VERSION_V2:
            return

        self.logger.debug('Creating minion {fqdn} in group {group}'.format(fqdn=fqdn, group=group))
        return self._get_deploy_api(DEPLOY_VERSION_V2).create_minion(fqdn, group)

    def create_minion_region_aware(self, fqdn, region_id):
        """
        Creates minion in new deploy system
        """

        if self.new_minions_deploy_version != DEPLOY_VERSION_V2:
            return

        self.logger.debug('Creating minion {fqdn} in region {region_id}'.format(fqdn=fqdn, region_id=region_id))
        return self._get_deploy_api(DEPLOY_VERSION_V2).create_minion_region_aware(fqdn, region_id)

    def delete_minion(self, fqdn):
        """
        Deletes minion from new deploy system
        """
        self.logger.debug('Deleting minion {fqdn}'.format(fqdn=fqdn))
        return self._get_deploy_api(DEPLOY_VERSION_V2).delete_minion(fqdn)

    def unregister_minion(self, fqdn):
        """
        Deletes key minion from deploy system
        """
        return self._get_deploy_api(DEPLOY_VERSION_V2).unregister_minion(fqdn)

    def has_minion(self, fqdn):
        """
        Checks if deploy v2 has this minion
        """
        return self._get_deploy_api(DEPLOY_VERSION_V2).has_minion(fqdn)

    def wait_minions_registered(self, *fqdns):
        """
        Wait until minions are registered in salt master
        """
        return self._get_deploy_api(DEPLOY_VERSION_V2).wait_minions_registered(*fqdns)

    def get_deploy_version_from_minion(self, fqdn: str) -> int:
        """
        Retrieve deploy version for specified minion
        """
        return DEPLOY_VERSION_V2

    def get_shipment_jobs(self, shipment_id) -> JobsListRespType:
        return self._get_deploy_api(DEPLOY_VERSION_V2)._get_shipment_jobs(shipment_id)

    def get_job_results(self, fqdn, ext_id) -> JobResultsListRespType:
        job_results = self._get_deploy_api(DEPLOY_VERSION_V2)._job_results(fqdn, ext_id)
        for result in job_results['jobResults']:
            if 'result' not in result:
                continue  # skip states with status='unknown'
            # on-the-wire field 'result' is base64-encoded json.
            # However, in our code it is exposed as decoded Dict[str, Any]
            raw: str = result['result']  # type: ignore
            result_bytes = base64.decodebytes(bytes(raw, 'utf8'))
            result_data = json.loads(result_bytes)
            result['result'] = result_data  # override 'result' field
        return job_results

    def get_job_results_for_shipment(self, shipment_id: Jid, fqdn: str) -> list[JobResultsListRespType]:
        result = []
        jobs = self.get_shipment_jobs(shipment_id)
        for job in jobs['jobs']:
            jobresult = self.get_job_results(fqdn, job['extId'])
            result.append(jobresult)
        return result

    def get_salt_state_result_for_shipment(self, shipment_id: Jid, fqdn: str, state_name: str) -> Union[dict, None]:
        """
        DEPRECATED. DON'T USE IT.
        Salt state output parsing is not reliable by design.

        Use get_result_for_shipment() with explicit salt module method call:
        invoke _call_salt_module_host()
         * with your custom `state_type` (e.g. 'mysql_mdb.check_upgrade')
         * and put method args to `operation` (e.g. "target_version='8.0.25'")
        This would be equivalent to
        # salt-call mysql_mdb.check_upgrade target_version='8.0.25'
        """
        job_results = self.get_job_results_for_shipment(shipment_id, fqdn)
        for jr in job_results:
            # there may be multiple jobresults for a single shipment_id
            # start lookup from newest to oldest (based on id order):
            salt_jrs = jr.get('jobResults', [])
            salt_jrs = sorted(salt_jrs, key=itemgetter('id'), reverse=True)
            for salt_jr in salt_jrs:
                result_data = salt_jr.get('result')
                if result_data is None:
                    continue
                if not result_data.get('fun', '').startswith('state.'):
                    continue  # nobody wants to see 'saltutil.sync_all' or non-salt deploys
                res = result_data.get('return', {})
                if type(res) != dict:
                    continue
                for k, v in res.items():
                    if v.get('__id__', None) == state_name:
                        return v
        return None

    def get_result_for_shipment(self, shipment_id: Jid, fqdn: str, state_type: str) -> Optional[dict]:
        job_results = self.get_job_results_for_shipment(shipment_id, fqdn)
        for jr in job_results:
            # there may be multiple jobresults for a single shipment_id
            # start lookup from newest to oldest (based on id order):
            jrs = jr.get('jobResults', [])
            jrs = sorted(jrs, key=itemgetter('id'), reverse=True)
            for last in jrs:
                result_data = last.get('result')
                if result_data is not None and result_data.get('fun') == state_type:
                    return result_data.get('return')
        return None

    def _get_deploy_version_from_jid(self, jid: Jid) -> int:
        return DEPLOY_VERSION_V2

    def _get_deploy_api(self, deploy_version):
        return self._deploy_apis[deploy_version]


def deploy_dataplane_host(config):
    deploy_host = config.deploy.dataplane_host_v2
    if not deploy_host:
        deploy_host = urllib.parse.urlparse(config.deploy.url_v2).netloc
    return deploy_host
