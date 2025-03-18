import os
import re
import tempfile
import subprocess
import multiprocessing
from io import BytesIO
from queue import Empty
from datetime import datetime, timedelta

from library.python import resource


class Worker(multiprocessing.Process):

    def __init__(self, _in, _out, **kwargs):
        super().__init__(**kwargs)
        self.in_queue = _in
        self.out_queue = _out
        self._stopped = False


    def run(self):
        """Main loop for thread, trying queue.get"""
        while not self._stopped:
            try:
                n, _it = self.in_queue.get(timeout=0.5)
                output = BytesIO()
                i = 0
                l = len(_it)
                while i < l:
                    i += 2
                    uid = _it[i:i + 8]
                    output.write(uid)
                    i += 8
                self.out_queue.put((n, output.getvalue()))
                output.close()
            except Empty:
                continue

    def stop(self):
        self._stopped = True


def place_matplotlibrc():
    # won't import matplotlib(<-pandas<-catboost) without it
    text = bytes.decode(resource.find("matplotlibrc"))
    filename = 'matplotlibrc'
    with open(filename, 'w') as f:
        f.write(text)
    os.environ['MATPLOTLIBRC'] = filename


def upload_file_to_sandbox(filename, resource_type='OTHER_RESOURCE', description=f'loaded from {__file__}', ttl='14'):
    from antiadblock.tasks.tools.logger import create_logger

    logger = create_logger(__file__)
    sb_token = os.getenv('SB_TOKEN')

    logger.info('Get Ya Tool')
    dirname = os.path.dirname(filename)
    ya_tool = os.path.join(dirname, 'ya')
    subprocess.call(['curl', 'https://api-gotya.n.yandex-team.ru/ya', '-o', ya_tool])
    subprocess.call(['chmod', '+x', ya_tool])

    logger.info(f'uploading no {filename} as SB resource')
    upload_result = subprocess.check_output([
        ya_tool, 'upload',
        '--type', resource_type,
        '--ttl', ttl,
        '--owner', 'ANTIADBLOCK',
        '--description', description,
        '--token', sb_token,
        filename,
    ], stderr=subprocess.STDOUT)
    logger.info(f'got response {upload_result}')
    resource_id = re.search(r'Created resource id is (\d+)', bytes.decode(upload_result)).groups()[0]
    logger.info(f'created new resource {resource_type} {resource_id}')
    return resource_id


def load_file_from_sandbox(resource_id, filename):
    from antiadblock.tasks.tools.logger import create_logger

    logger = create_logger(__file__)
    logger.info(f'loading SB recource {resource_id} to {filename}')
    resp = subprocess.call([
        'curl', f'https://proxy.sandbox.yandex-team.ru/{resource_id}',
        '-o',
        filename,
    ], stderr=subprocess.STDOUT)
    logger.info(f'loading result {resp}')


def save_uids_to_file(yt_client, filename, table_path, limit=0):
    import yt.wrapper as yt
    from antiadblock.tasks.tools.logger import create_logger

    logger = create_logger(__file__)

    row_count = yt_client.row_count(table_path)
    if limit <= row_count:
        start = row_count - limit
        table_path = f'{table_path}[#{start}:]'
        logger.info(f'Limit number uids: {limit}')

    table_skiff_schemas = [
        {
            "wire_type": "tuple",
            "children": [
                {
                    'name': 'uniqid',
                    'wire_type': 'uint64',
                }
            ]
        }
    ]
    skiff_format = yt.SkiffFormat(table_skiff_schemas)
    logger.info('Start read table from YT')
    iterable = yt_client.read_table(table_path, raw=True, format=skiff_format, enable_read_parallel=True)
    logger.info('End read table from YT')

    in_queue = multiprocessing.Queue()
    out_queue = multiprocessing.Queue()
    yt_worker_count = int(os.getenv('YT_WORKER_COUNT', '8'))
    pool = [Worker(in_queue, out_queue, name=f'Process {i}') for i in range(yt_worker_count)]
    for p in pool:
        p.start()

    cit = iterable.chunk_iter()
    data = {}
    logger.info('Start read chunks')
    chunk_cnt = 0
    for chunk in cit:
        in_queue.put((chunk_cnt, chunk))
        chunk_cnt += 1
    logger.info(f'Count chunks: {chunk_cnt}, getting data from queue')
    get_data_start_dt = datetime.now()
    is_complete = True
    while chunk_cnt > 0:
        if datetime.now() - get_data_start_dt > timedelta(minutes=5):
            is_complete = False
            break
        i, chunk_data = out_queue.get()
        data[i] = chunk_data
        chunk_cnt -= 1

    if is_complete:
        with open(filename, 'wb') as f:
            for _, chunk_data in sorted(data.items()):
                f.write(chunk_data)

    for p in pool:
        p.stop()
        p.join(timeout=0.5)
        if p.is_alive():
            p.terminate()
    if not is_complete:
        logger.error('Failed to process data in the allotted time')
        exit(-1)

def upload_uniqids(yt_client, table_path, resource_type, limit=0, ttl='14'):
    from antiadblock.tasks.tools.logger import create_logger

    logger = create_logger(__file__)

    tmp_dir = tempfile.mkdtemp()
    logger.info(f'Created tmp dir {tmp_dir}')
    bypass_uids_file = os.path.join(tmp_dir,  resource_type)
    logger.info('Downloading no adblock uids')
    save_uids_to_file(yt_client, bypass_uids_file, table_path, limit)
    logger.info(f'No adblock uids downloaded from {table_path} to {bypass_uids_file}')

    upload_file_to_sandbox(bypass_uids_file, resource_type, description=f'uids without Adblock {table_path}', ttl=ttl)

    logger.info('Removing temp directory')
    try:
        for tmp_file in os.listdir(tmp_dir):
            os.remove(os.path.join(tmp_dir, tmp_file))
        os.rmdir(tmp_dir)
    except OSError:
        logger.error(f"Deletion of the directory {tmp_dir} failed")
    else:
        logger.error(f"Successfully deleted the directory {tmp_dir}")

def create_clients(token, db):
    import yt.wrapper as yt
    from yql.api.v1.client import YqlClient

    yql_client = YqlClient(token=token, db=db)
    yt_client = yt.YtClient(
        token=token,
        proxy=db,
        config=dict(read_parallel=dict(enable=True, max_thread_count=56)),
    )
    return yql_client, yt_client
