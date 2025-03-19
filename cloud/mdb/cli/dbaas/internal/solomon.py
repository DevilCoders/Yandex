from cloud.mdb.cli.dbaas.internal.config import config_option
from cloud.mdb.cli.dbaas.internal.rest import rest_request


def get_shards(ctx, limit=1000):
    project = config_option(ctx, 'solomon', 'project')
    params = {
        'pageSize': limit,
    }
    return _request(ctx, 'GET', f'projects/{project}/shards', params=params)['result']


def get_clusters(ctx, limit=1000):
    project = config_option(ctx, 'solomon', 'project')
    params = {
        'pageSize': limit,
    }
    return _request(ctx, 'GET', f'projects/{project}/clusters', params=params)['result']


def _request(ctx, method, url_path, **kwargs):
    return rest_request(ctx, 'solomon', method, f'/api/v2/{url_path}', **kwargs)
