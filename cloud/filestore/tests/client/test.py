import json
import os

import yatest.common as common

from cloud.filestore.tests.python.lib.client import NfsCliClient

BLOCK_SIZE = 4 * 1024
BLOCKS_COUNT = 1000


def __init_test():
    port = os.getenv("NFS_SERVER_PORT")
    binary_path = common.binary_path("cloud/filestore/client/filestore-client")
    client = NfsCliClient(binary_path, port, cwd=common.output_path())

    results_path = common.output_path() + "/results.txt"
    return client, results_path


def __serialize_ls_output(lines):
    serialized = ""
    for line in lines:
        j = json.dumps(line[1])
        serialized += line[0] + '\t' + j
        serialized += '\n'

    serialized.encode('utf-8')
    return serialized


def __exec_ls(client, *args):
    output = str(client.ls(*args), 'utf-8')

    lines = output.splitlines()
    first = [x.split('\t')[0] for x in lines]
    second = [x.split('\t')[1] for x in lines]

    parsed = []
    for i in range(len(first)):
        parsed.append((first[i], json.loads(second[i])))

    for line in parsed:
        del line[1]["ATime"]
        del line[1]["MTime"]
        del line[1]["CTime"]

    return __serialize_ls_output(lines).encode('utf-8')


def test_create_destroy():
    client, results_path = __init_test()

    out = client.create("fs0", "test_cloud", "test_folder", BLOCK_SIZE, BLOCKS_COUNT)
    out += client.destroy("fs0")

    with open(results_path, "wb") as results_file:
        results_file.write(out)

    ret = common.canonical_file(results_path)
    return ret


def test_create_mkdir_ls_destroy():
    client, results_path = __init_test()

    out = client.create("fs0", "test_cloud", "test_folder", BLOCK_SIZE, BLOCKS_COUNT)

    client.mkdir("fs0", "/aaa")
    client.mkdir("fs0", "/bbb")

    out += __exec_ls(client, "fs0", "/")
    out += client.destroy("fs0")

    with open(results_path, "wb") as results_file:
        results_file.write(out)

    ret = common.canonical_file(results_path)
    return ret


def test_create_mkdir_ls_write_destroy():
    client, results_path = __init_test()

    out = client.create("fs0", "test_cloud", "test_folder", BLOCK_SIZE, BLOCKS_COUNT)

    client.mkdir("fs0", "/aaa")
    client.touch("fs0", "/first")
    out += __exec_ls(client, "fs0", "/")
    out += client.destroy("fs0")

    with open(results_path, "wb") as results_file:
        results_file.write(out)

    ret = common.canonical_file(results_path)
    return ret


def test_list_filestores():
    client, results_path = __init_test()

    out = client.create("fs0", "test_cloud", "test_folder", BLOCK_SIZE, BLOCKS_COUNT)
    out += client.create("fs1", "test_cloud", "test_folder", BLOCK_SIZE, BLOCKS_COUNT)
    out += client.create("fs2", "test_cloud", "test_folder", BLOCK_SIZE, BLOCKS_COUNT)
    out += client.create("fs3", "test_cloud", "test_folder", BLOCK_SIZE, BLOCKS_COUNT)

    out += ",".join(client.list_filestores()).encode()

    out += client.destroy("fs3")
    out += client.destroy("fs2")
    out += client.destroy("fs1")
    out += client.destroy("fs0")

    with open(results_path, "wb") as results_file:
        results_file.write(out)

    ret = common.canonical_file(results_path)
    return ret
