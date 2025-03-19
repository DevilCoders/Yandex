"""
Simple dbm mock
"""

import json

from httmock import urlmatch

from .utils import handle_http_action, http_return


def dbm(state):
    """
    Setup mock with state
    """

    @urlmatch(netloc='dbm.test', method='get', path='/api/v2/transfers/')
    def get_transfer(url, _):
        transfer_id = url.path.split('/')[-1]
        if not transfer_id:
            fqdn = url.query.replace('fqdn=', '')
            ret = handle_http_action(state, f'dbm-transfer-get-{fqdn}')
            if ret:
                return ret

            body = {}
            for data in state['dbm']['transfers'].values():
                if data['container'] == fqdn:
                    body = data
                    break

            return http_return(body=body)

        ret = handle_http_action(state, f'dbm-transfer-get-{transfer_id}')
        if ret:
            return ret

        transfer = state['dbm']['transfers'].get(transfer_id)

        if not transfer:
            return http_return(code=404, body={'error': 'No such transfer'})

        return http_return(body=transfer)

    @urlmatch(netloc='dbm.test', method='get', path='/api/v2/containers/.+')
    def get_container(url, _):
        fqdn = url.path.split('/')[-1]
        ret = handle_http_action(state, f'dbm-container-get-{fqdn}')
        if ret:
            return ret

        container = state['dbm']['containers'].get(fqdn)

        if container is None:
            return http_return(code=404, body={'error': 'No such container'})

        return http_return(body=container['container'])

    @urlmatch(netloc='dbm.test', method='get', path='/api/v2/volumes/.+')
    def get_volumes(url, _):
        fqdn = url.path.split('/')[-1]
        ret = handle_http_action(state, f'dbm-volumes-get-{fqdn}')
        if ret:
            return ret

        container = state['dbm']['containers'].get(fqdn)

        if container is None:
            return http_return(code=404, body={'error': 'No such container'})

        return http_return(body=container['volumes'])

    @urlmatch(netloc='dbm.test', method='put', path='/api/v2/containers/.+')
    def create_container(url, request):
        fqdn = url.path.split('/')[-1]
        ret = handle_http_action(state, f'dbm-container-create-{fqdn}')
        if ret:
            return ret

        container = json.loads(request.body)

        if state['dbm']['containers'].get(fqdn):
            return http_return(code=400, body={'error': 'Container already exists'})

        container['dom0'] = f'{container["geo"]}-{container["generation"]}'

        state['dbm']['containers'][fqdn] = {
            'container': {k: v for k, v in container.items() if k not in ('secrets', 'volumes')},
            'volumes': container['volumes'],
        }

        shipment_id = f'{container["dom0"]}-create-{fqdn}'

        state['deploy-v2']['shipments'][shipment_id] = {'status': 'done', 'id': shipment_id}

        return http_return(body={'deploy': {'deploy_id': shipment_id, 'deploy_version': 2, 'host': container['dom0']}})

    @urlmatch(netloc='dbm.test', method='post', path='/api/v2/containers/.+')
    def modify_container(url, request):
        fqdn = url.path.split('/')[-1]
        ret = handle_http_action(state, f'dbm-container-modify-{fqdn}')
        if ret:
            return ret

        data = json.loads(request.body)

        container = state['dbm']['containers'].get(fqdn)

        if container is None:
            return http_return(code=404, body={'error': 'No such container'})

        if 'generation' in data and container['container']['generation'] != data['generation'] or 'dom0' in data:
            container['container'].update(data)
            new_dom0 = data['dom0'] if 'dom0' in data else f'{container["container"]["geo"]}-{data["generation"]}'
            transfer_id = str(max([int(i['id']) for i in state['dbm']['transfers'].values()] + [0]) + 1)
            state['dbm']['transfers'][transfer_id] = {
                'id': transfer_id,
                'container': fqdn,
                'src_dom0': container['container']['dom0'],
                'dest_dom0': new_dom0,
            }
            return http_return(body={'transfer': transfer_id})

        container['container'].update(data)
        shipment_id = f'{container["container"]["dom0"]}-modify-{fqdn}'
        state['deploy-v2']['shipments'][shipment_id] = {'status': 'done', 'id': shipment_id}
        return http_return(
            body={'deploy': {'deploy_id': shipment_id, 'deploy_version': 2, 'host': container['container']['dom0']}}
        )

    @urlmatch(netloc='dbm.test', method='post', path='/api/v2/volumes/.+')
    def modify_volume(url, request):
        fqdn = url.path.split('/')[-1]
        path = url.query.replace('path=', '')
        ret = handle_http_action(state, f'dbm-volume-modify-{fqdn}-{path}')
        if ret:
            return ret

        data = json.loads(request.body)

        container = state['dbm']['containers'].get(fqdn)

        if container is None:
            return http_return(code=404, body={'error': 'No such container'})

        for volume in container['volumes']:
            if volume['path'] == path:
                volume.update(data)
                shipment_id = f'{container["container"]["dom0"]}-modify-{fqdn}-volume'
                state['deploy-v2']['shipments'][shipment_id] = {'status': 'done', 'id': shipment_id}
                return http_return(
                    body={
                        'deploy': {
                            'deploy_id': shipment_id,
                            'deploy_version': 2,
                            'host': container['container']['dom0'],
                        }
                    }
                )

        return http_return(code=404, body={'error': 'No such volume'})

    @urlmatch(netloc='dbm.test', method='delete', path='/api/v2/containers/.+')
    def delete_container(url, _):
        fqdn = url.path.split('/')[-1]
        ret = handle_http_action(state, f'dbm-container-delete-{fqdn}')
        if ret:
            return ret

        container = state['dbm']['containers'].get(fqdn)

        if container is None:
            return http_return(code=404, body={'error': 'No such container'})

        shipment_id = f'{container["container"]["dom0"]}-delete-{fqdn}'
        state['deploy-v2']['shipments'][shipment_id] = {'status': 'done', 'id': shipment_id}
        del state['dbm']['containers'][fqdn]
        return http_return(
            body={'deploy': {'deploy_id': shipment_id, 'deploy_version': 2, 'host': container['container']['dom0']}}
        )

    @urlmatch(netloc='dbm.test', method='post', path='/api/v2/transfers/.+/finish')
    def finish_transfer(url, _):
        transfer_id = url.path.split('/')[-2]
        ret = handle_http_action(state, f'dbm-finish-transfer-{transfer_id}')
        if ret:
            return ret

        transfer = state['dbm']['transfers'].get(transfer_id)
        if not transfer:
            return http_return(code=404, body={'error': 'No such transfer'})

        transfer['is_finished'] = True

        return http_return(body={})

    return (
        get_transfer,
        get_container,
        get_volumes,
        create_container,
        modify_container,
        modify_volume,
        delete_container,
        finish_transfer,
    )
