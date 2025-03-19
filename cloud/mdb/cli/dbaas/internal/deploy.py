import base64
import json
from datetime import datetime

from cloud.mdb.cli.dbaas.internal.common import to_overlay_fqdn, to_underlay_fqdn, to_underlay_fqdn_list
from cloud.mdb.cli.dbaas.internal.rest import rest_request


def get_deploy_group(ctx, group_name):
    """
    Get deploy group.
    """
    return _get_request(ctx, f'/v1/groups/{group_name}')


def get_deploy_groups(ctx, limit=1000):
    """
    List deploy groups.
    """
    params = {
        'pageSize': limit,
    }

    return _list_request(ctx, '/v1/groups', 'groups', params=params)


def create_deploy_group(ctx, group_name):
    """
    Create deploy group.
    """
    data = {
        'name': group_name,
    }
    return _post_request(ctx, '/v1/groups', data=data)


def delete_deploy_group(ctx, group_name):
    """
    Delete deploy group.
    """
    return _delete_request(ctx, f'/v1/groups/{group_name}')


def get_master(ctx, fqdn):
    """
    Get master.
    """
    fqdn = to_overlay_fqdn(ctx, fqdn)
    return _get_request(ctx, f'/v1/masters/{fqdn}', parser=_parse_master)


def get_masters(ctx, limit=1000):
    """
    List masters.
    """
    params = {
        'pageSize': limit,
    }

    return _list_request(ctx, '/v1/masters', 'masters', params=params, parser=_parse_master)


def _parse_master(ctx, master):
    master['fqdn'] = to_underlay_fqdn(ctx, master['fqdn'])
    master['aliveCheckAt'] = datetime.fromtimestamp(master['aliveCheckAt'])
    master['createdAt'] = datetime.fromtimestamp(master['createdAt'])
    return master


def get_minion(ctx, fqdn):
    """
    Get minion.
    """
    fqdn = to_overlay_fqdn(ctx, fqdn)
    return _get_request(ctx, f'/v1/minions/{fqdn}', parser=_parse_minion)


def get_minions(ctx, limit=1000):
    """
    List minions.
    """
    params = {
        'pageSize': limit,
    }

    return _list_request(ctx, '/v1/minions', 'minions', params=params, parser=_parse_minion)


def upsert_minion(ctx, fqdn, deploy_group, autoreassign):
    """
    Create new minion or update the existing one.
    """
    fqdn = to_overlay_fqdn(ctx, fqdn)
    data = {
        'fqdn': fqdn,
        'group': deploy_group,
        'autoReassign': autoreassign,
    }
    return _put_request(ctx, f'/v1/minions/{fqdn}', data=data)


def unregister_minion(ctx, fqdn):
    """
    Unregister minion.
    """
    fqdn = to_overlay_fqdn(ctx, fqdn)
    return _post_request(ctx, f'/v1/minions/{fqdn}/unregister')


def delete_minion(ctx, fqdn, suppress_errors=False):
    """
    Delete minion.
    """
    fqdn = to_overlay_fqdn(ctx, fqdn)
    return _delete_request(ctx, f'/v1/minions/{fqdn}', suppress_errors=suppress_errors)


def _parse_minion(ctx, minion):
    minion['fqdn'] = to_underlay_fqdn(ctx, minion['fqdn'])
    minion['createdAt'] = datetime.fromtimestamp(minion['createdAt'])
    minion['updatedAt'] = datetime.fromtimestamp(minion['updatedAt'])
    return minion


def get_shipment(ctx, shipment_id):
    """
    Get shipment by ID.
    """
    return _get_request(ctx, f'/v1/shipments/{shipment_id}', parser=_parse_shipment)


def get_shipments(ctx, *, fqdn=None, sort_order='desc', limit=1000):
    """
    List shipments.
    """
    params = {
        'pageSize': limit,
        'sortOrder': sort_order,
    }
    if fqdn:
        params['fqdn'] = to_overlay_fqdn(ctx, fqdn)

    return _list_request(ctx, '/v1/shipments', 'shipments', params=params, parser=_parse_shipment)


def _parse_shipment(ctx, shipment):
    shipment['fqdns'] = to_underlay_fqdn_list(ctx, shipment['fqdns'])
    shipment['createdAt'] = datetime.fromtimestamp(shipment['createdAt'])
    shipment['updatedAt'] = datetime.fromtimestamp(shipment['updatedAt'])
    return shipment


def get_command(ctx, command_id=None):
    """
    Get deploy command by ID.
    """
    return _get_request(ctx, f'/v1/commands/{command_id}', parser=_parse_command)


def get_commands(ctx, *, shipment_id=None, sort_order='desc', limit=1000):
    """
    List deploy commands.
    """
    params = {
        'pageSize': limit,
        'sortOrder': sort_order,
    }
    if shipment_id:
        params['shipmentId'] = shipment_id

    return _list_request(ctx, '/v1/commands', 'commands', params=params, parser=_parse_command)


def _parse_command(ctx, command):
    command['fqdn'] = to_underlay_fqdn(ctx, command['fqdn'])
    command['createdAt'] = datetime.fromtimestamp(command['createdAt'])
    command['updatedAt'] = datetime.fromtimestamp(command['updatedAt'])
    return command


def get_job(ctx, job_id):
    """
    Get deploy job by ID.
    """
    return _get_request(ctx, f'/v1/jobs/{job_id}', parser=_parse_job)


def get_jobs(ctx, *, shipment_id=None, command_id=None, sort_order='desc', limit=1000):
    """
    List deploy jobs.
    """
    params = {
        'pageSize': limit,
        'sortOrder': sort_order,
    }
    if shipment_id:
        params['shipmentId'] = shipment_id

    jobs = _list_request(ctx, '/v1/jobs', 'jobs', params=params, parser=_parse_job)

    if command_id:
        jobs = filter_jobs(jobs, command_id=command_id)

    return jobs


def filter_jobs(jobs, command_id=None):
    result = []
    for job in jobs:
        if command_id and job['commandID'] != command_id:
            continue

        result.append(job)

    return result


def _parse_job(ctx, job):
    job['createdAt'] = datetime.fromtimestamp(job['createdAt'])
    job['updatedAt'] = datetime.fromtimestamp(job['updatedAt'])
    return job


def get_job_result(ctx, job_result_id):
    """
    Get job result by ID.
    """
    return _get_request(ctx, f'/v1/jobresults/{job_result_id}', parser=_parse_job_result)


def get_job_results(ctx, fqdn=None, ext_job_id=None, limit=1000):
    """
    List job results.
    """
    params = {
        'limit': limit,
    }
    if fqdn:
        params['fqdn'] = to_overlay_fqdn(ctx, fqdn)
    if ext_job_id:
        params['jobId'] = ext_job_id

    return _list_request(ctx, '/v1/jobresults', 'jobResults', params=params, parser=_parse_job_result)


def _parse_job_result(ctx, result):
    result['recordedAt'] = datetime.fromtimestamp(result['recordedAt'])
    result['result'] = json.loads(base64.b64decode(result['result']))
    return result


def _get_request(ctx, url_path, parser=None):
    response = rest_request(ctx, 'deploy', 'GET', url_path)
    return parser(ctx, response) if parser else response


def _list_request(ctx, url_path, result_field, params, parser=None):
    response = rest_request(ctx, 'deploy', 'GET', url_path, params=params)
    obj_list = response[result_field] or []
    return [parser(ctx, obj) for obj in obj_list] if parser else obj_list


def _post_request(ctx, url_path, data=None, suppress_errors=False):
    response = rest_request(ctx, 'deploy', 'POST', url_path, data=data, suppress_errors=suppress_errors)
    return response


def _put_request(ctx, url_path, data, suppress_errors=False):
    response = rest_request(ctx, 'deploy', 'PUT', url_path, data=data, suppress_errors=suppress_errors)
    return response


def _delete_request(ctx, url_path, suppress_errors=False):
    response = rest_request(ctx, 'deploy', 'DELETE', url_path, suppress_errors=suppress_errors)
    return response
