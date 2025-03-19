import requests

path_prefix = 'arcadia/'


# path should start from 'arcadia', i.e. 'arcadia/cloud/ai/...'
def arcadia_download(path: str, revision: int, token: str) -> str:
    if not path.startswith(path_prefix):
        raise ValueError(f'Path should start from "{path_prefix}"')

    resp = requests.get(
        f'https://a.yandex-team.ru/api/tree/blob/trunk/{path}',
        params={'rev': revision},
        headers={'Authorization': f'OAuth {token}'},
    )

    resp.raise_for_status()

    return resp.content.decode('utf8')
