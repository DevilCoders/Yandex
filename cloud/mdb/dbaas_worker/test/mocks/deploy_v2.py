"""
Simple deploy-v2 mock
"""

import json
import urllib.parse

from httmock import urlmatch

from .utils import handle_http_action, http_return


def deploy_v2(state):
    """
    Setup mock with state
    """
    record_all_shipments = state['deploy-v2']['record_all_shipments']

    @urlmatch(netloc='deploy-v2.test', method='get', path='/v1/minions/.+')
    def get_minion(url, _):
        minion_id = url.path.split('/')[-1]
        ret = handle_http_action(state, f'deploy-v2-minion-get-{minion_id}')
        if ret:
            return ret

        if minion_id in state['deploy-v2']['minions']:
            return http_return(body=state['deploy-v2']['minions'][minion_id])

        return http_return(code=404, body={'message': 'data not found'})

    @urlmatch(netloc='deploy-v2.test', method='put', path='/v1/minions/.+')
    def create_minion(url, request):
        minion_id = url.path.split('/')[-1]
        data = json.loads(request.body)
        ret = handle_http_action(state, f'deploy-v2-minion-create-{minion_id}')
        if ret:
            return ret

        data['registered'] = True
        state['deploy-v2']['minions'][minion_id] = data
        return http_return(body=data)

    @urlmatch(netloc='deploy-v2.test', method='post', path='/v1/minions/.+/unregister')
    def unregister_minion(url, request):
        minion_id = url.path.split('/')[-2]
        ret = handle_http_action(state, f'deploy-v2-minion-unregister-{minion_id}')
        if ret:
            return ret

        if minion_id in state['deploy-v2']['minions']:
            return http_return()

        return http_return(code=404, body={'message': 'minion not created'})

    @urlmatch(netloc='deploy-v2.test', method='delete', path='/v1/minions/.+')
    def delete_minion(url, _):
        minion_id = url.path.split('/')[-1]
        ret = handle_http_action(state, f'deploy-v2-minion-delete-{minion_id}')
        if ret:
            return ret

        if minion_id in state['deploy-v2']['minions']:
            return http_return(body=state['deploy-v2']['minions'].pop(minion_id))

        return http_return(code=404, body={'message': 'data not found'})

    @urlmatch(netloc='deploy-v2.test', method='get', path='/v1/shipments/.+')
    def get_shipment(url, _):
        shipment_id = url.path.split('/')[-1]
        ret = handle_http_action(state, f'deploy-v2-shipment-get-{shipment_id}')
        if ret:
            return ret

        if shipment_id in state['deploy-v2']['shipments']:
            return http_return(body=state['deploy-v2']['shipments'][shipment_id])

        return http_return(code=404, body={'message': 'data not found'})

    @urlmatch(netloc='deploy-v2.test', method='post', path='/v1/shipments')
    def create_shipment(_, request):
        data = json.loads(request.body)
        commands = '-'.join(x['type'] for x in data['commands'] if x['type'] != 'saltutil.sync_all')
        shipment_id = f'{data["fqdns"][0]}-{commands}'

        if record_all_shipments:
            prefix = shipment_id
            suffix_num = 2

            while shipment_id in state['deploy-v2']['shipments']:
                suffix_num += 1
                shipment_id = f'{prefix}-{suffix_num}'

        ret = handle_http_action(state, f'deploy-v2-shipment-create-{shipment_id}')
        if ret:
            return ret

        state['deploy-v2']['shipments'][shipment_id] = data
        state['deploy-v2']['shipments'][shipment_id]['id'] = shipment_id
        state['deploy-v2']['shipments'][shipment_id]['status'] = 'done'

        return http_return(body=state['deploy-v2']['shipments'][shipment_id])

    @urlmatch(netloc='deploy-v2.test', method='get', path='/v1/jobs')
    def get_jobs(url, request):
        shipment_id = url.query.split('=')[1]

        if 'jobs' in state['deploy-v2'] and shipment_id in state['deploy-v2']['jobs']:
            return http_return(body={'jobs': state['deploy-v2']['jobs'][shipment_id]})

        return http_return(code=404, body={'message': 'jobs not found for shipment_id=' + shipment_id})

    @urlmatch(netloc='deploy-v2.test', method='get', path='/v1/jobresults')
    def get_jobresults(url, request):
        qp = urllib.parse.parse_qs(url.query)
        ext_job_id = qp['extJobId'][0]
        fqdn = qp['fqdn'][0]

        if 'jobresults' not in state['deploy-v2']:
            return http_return(code=404, body={'message': 'jobresults not intialized'})

        for jr in state['deploy-v2']['jobresults']:
            if jr['fqdn'] == fqdn and jr['extID'] == ext_job_id:
                return http_return(body={'jobResults': [jr]})

        return http_return(
            code=404, body={'message': 'jobresults not found for ext_job_id=' + ext_job_id + ', fqdn=' + fqdn}
        )

    return (
        get_minion,
        create_minion,
        delete_minion,
        get_shipment,
        create_shipment,
        unregister_minion,
        get_jobs,
        get_jobresults,
    )
