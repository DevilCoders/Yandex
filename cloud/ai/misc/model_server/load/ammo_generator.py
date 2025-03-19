# coding: utf-8

# Based on https://yandextank.readthedocs.io/en/latest/ammo_generators.html

import os
import requests


def print_request(request):
    req = "{method} {path_url} HTTP/1.1\r\n{headers}\r\n{body}".format(
        method=request.method,
        path_url=request.path_url,
        headers=''.join('{0}: {1}\r\n'.format(k, v) for k, v in request.headers.items()),
        body=request.body or "",
    )
    return "{req_size}\n{req}\r\n".format(req_size=len(req), req=req)


# POST multipart form data
def post_multipart(host, port, namespace, files, headers, payload):
    req = requests.Request(
        'POST',
        'http://{host}:{port}{namespace}'.format(
            host=host,
            port=port,
            namespace=namespace,
        ),
        headers=headers,
        data=payload,
        files=files
    )
    prepared = req.prepare()
    return print_request(prepared)


if __name__ == "__main__":
    for filename in os.listdir('./images'):
        if not filename.endswith('jpg'):
            continue
        host = 'example.com'
        port = '80'
        namespace = '/v1/classify'
        headers = {'Host': host}
        payload = {'model': 'badoo'}
        files = {
            # name, path_to_file, content-type, additional headers
            'data': (filename, open('./images/%s' % filename, 'rb'), 'image/jpeg ', {'Expires': '0'})
        }

        print post_multipart(host, port, namespace, files, headers, payload)
