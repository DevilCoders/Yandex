#!/usr/bin/env python
# coding: utf-8
from subprocess import check_output, STDOUT, CalledProcessError
import requests
from tempfile import mkdtemp
from contextlib import contextmanager
import shutil
import sys
import os
import hashlib
import re
import time
import uuid


@contextmanager
def tmpdir(*args, **kwargs):
    old_dir = os.getcwd()
    tmp_path = mkdtemp(*args, **kwargs)
    os.chmod(tmp_path, 0o755)  # workaround for skynet wich tries to open downloaded files by skybone daemon
    os.chdir(tmp_path)
    try:
        yield tmp_path
    finally:
        os.chdir(old_dir)
        shutil.rmtree(tmp_path)


def md5_file(path):
    hash_md5 = hashlib.md5()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()


def check_skynet_logs(md5):
    # super dumb!
    time.sleep(2)  # sky get can return before "file done" appears in log
    output = check_output(['timetail', '-t', 'java', '-n', '60', '/var/log/skynet/skybone-download.log'])
    patterns = ['{}: file done'.format(md5), '{}, link: http://'.format(md5)]
    for pattern in patterns:
        assert re.search(pattern, output), '"{}" not found in skynet logs'.format(pattern)


def main():
    # 'http://localhost/s3_to_torrent' 'storage-admin/rbtorrent-check' 92b16bcf654b9e2f994afaf48e932d5b
    bucket, key, md5 = sys.argv[1:]
    path =  str(uuid.uuid4())
    data = [{
        "bucket": bucket,
        "s3_key": key,
        "type": "file",
        "path": path,
    }]

    status = 0
    msg = 'OK'
    req_id = '0'
    url = 'http://localhost/announce?max_age=1&sync=true&ttl=180'
    try:
        r = requests.post(url, json=data)
        req_id = r.headers['X-Request-Id']
        r.raise_for_status()
        rbtorrent_id = r.text.strip()
        with tmpdir() as tmp_path:
            check_output(['sky', 'get', rbtorrent_id], stderr=STDOUT)
            file_md5 = md5_file(path)
            assert md5 == file_md5, 'file_md5 {} != {} from commandline'.format(file_md5, md5)
        check_skynet_logs(md5)
        check_output(['/skynet/tools/skybone-ctl', 'notify', tmp_path])  # notify skynet to stop announcing
    except CalledProcessError as e:
        status = 2
        msg = '{} out={}'.format(e, e.output)
    except Exception as e:
        status = 2
        msg = str(e)
    print '{};{} req_id:{}'.format(status, msg, req_id).replace('\n', '---')
    sys.exit(status)


if __name__ == '__main__':
    main()
