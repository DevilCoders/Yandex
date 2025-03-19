#!/usr/bin/env python
"""
Get porto properties inside container
"""

import socket


def porto_resources():
    """
    Get resources of container via porto api
    """
    try:
        import porto

        conn = porto.Connection()
        conn.connect(timeout=1)

        container = conn.Find(socket.getfqdn())
        cleaned = {}
        for key, value in container.GetProperties().items():
            if not isinstance(value, Exception):
                cleaned[key] = value
        volumes = {}
        for volume in conn.GetVolumes():
            volumes[volume['path']] = volume
        # porto.Version() is a tuple of (tag, revision)
        # https://a.yandex-team.ru/svn/trunk/arcadia/infra/porto/proto/rpc.proto?rev=r8631203#L949
        # >>> conn.Version()
        # ('5.2.8', '')
        version = conn.Version()[0]
        return {'porto_resources': {'container': cleaned, 'volumes': volumes}, 'porto_version': version}
    except Exception:
        # We do not return an empty porto_version, because it will be more difficult to use it.
        # Calling grains.get('porto_version, '42') will return '' in such case
        return {'porto_resources': {}}


if __name__ == '__main__':
    import json

    print(json.dumps(porto_resources(), indent=4, sort_keys=True))
