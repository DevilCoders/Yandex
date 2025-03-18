#!/usr/bin/env python
# coding: utf-8
# pylint: disable=invalid-name
"""
A streamed (chunked) example of `simple_upload`.

WARNING: currently, this does not work as intended on the API side,
and, thus, there is still a 300MiB limit on the body size.
"""

from __future__ import division, absolute_import, print_function, unicode_literals

import os
import logging
import statface_client

logger = logging.getLogger('statface_client_examples')

if os.environ.get('SFCLIEX_LOGGING_CONVENIENCE'):
    from pyaux.runlib import init_logging
    init_logging(level=logging.DEBUG)
    logging.root.handlers[0].formatter._fmt += '\n'
else:
    logging.basicConfig(level=logging.DEBUG)


path = 'Adhoc/Adhoc/ExampleReport'
data = [
    dict(fielddate='2018-01-02', project='upgen01', value=2),
    dict(fielddate='2018-01-02', project='upgen02', value=3),
]
import ujson as json
data_chunk = b''.join(json.dumps(line).encode('utf-8') + b'\n' for line in data)
data_chunk = data_chunk * 5000  # 10k lines / chunk

chunk_size = len(data_chunk)
print('Chunk size:', chunk_size)
MiB = 2**20
full_size = 20 * MiB
chunk_count = int(full_size / len(data_chunk))
full_size_actual = len(data_chunk) * chunk_count


def make_data():
    try:
        import tqdm
    except Exception:
        for _ in range(chunk_count):
            yield data_chunk
    else:
        with tqdm.tqdm(total=full_size_actual, unit='B', unit_scale=True) as pbar:
            for _ in range(chunk_count):
                yield data_chunk
                pbar.update(chunk_size)

# sfcli = statface_client.ProductionStatfaceClient(_no_excess_calls=True)
sfcli = statface_client.BetaStatfaceClient(_no_excess_calls=True)
# sfcli = statface_client.StatfaceClient(host='http://sas2-8dd285c6898e.qloud-c.yandex.net', _no_excess_calls=True)
sfreport = sfcli.get_report(path)
# sfreport.simple_upload(scale='daily', data=data, streamed=True)
sfreport._api._report_post('simple_upload', method='put', params=dict(scale='daily', chunk_size=5000), data=make_data())
