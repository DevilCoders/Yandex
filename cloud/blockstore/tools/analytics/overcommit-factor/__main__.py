import argparse
import json
import numpy as np
import os
import pandas as pd
import requests

from cloud.blockstore.pylibs import common
from cloud.blockstore.pylibs.ycp import Ycp, make_ycp_engine
from concurrent.futures.thread import ThreadPoolExecutor
from datetime import datetime, timedelta
from multiprocessing import Lock

api_url = "https://solomon.yandex.net/api/v2/projects/nbs/sensors/data"
iops_per_host = 126000
bw_per_host = 2700*1024*1024

lock = Lock()
tasks_total = 0
tasks_completed = 0


def progress_indicator(future):
    global lock, tasks_total, tasks_completed
    with lock:
        tasks_completed = tasks_completed + 1
        if (tasks_completed % (tasks_total // 20)) == 0:
            current_time = datetime.now().strftime("%H:%M:%S")
            print(f'{current_time}: {tasks_completed}/{tasks_total} completed')


def fetch_scalar(cluster, service, instance_id, sensor, end, aux=[]):
    headers = {
        "Authorization": "OAuth " + os.environ["TOKEN"],
        "Content-Type": "application/json",
    }

    beg = end - timedelta(days=1)
    aux_str = "," + ",".join("%s=%s" % (x[0], x[1]) for x in aux) if len(aux) else ""
    data = {
        "program": "{cluster=yandexcloud_%s,service=%s,host=cluster,instance=%s,sensor=%s%s}" % (
            cluster,
            service,
            instance_id,
            sensor,
            aux_str
        ),
        "from": beg.strftime("%Y-%m-%dT%H:%M:%S.000Z"),
        "to": end.strftime("%Y-%m-%dT%H:%M:%S.000Z"),
        "downsampling": {
            "aggregation": "AVG",
            "disabled": False,
            "fill": "NULL",
            "gridMillis": 15000,
            "ignoreMinStepMillis": True,
            "maxPoints": 5760
        },
    }
    r = requests.post(api_url, headers=headers, data=json.dumps(data))
    result = r.json()
    return result['vector']


def parse_instances(cores_num, lines, coresDict):
    instanceDict = {}
    even = False
    for line in lines:
        even = not even

        line = line.strip()
        if even:
            prefix = "instance-id: "
        else:
            prefix = "cpu_unit_count: "

        if not line.startswith(prefix):
            raise Exception("failed format '%s'" % line)
        value = line[len(prefix):]

        if even:
            instance_id = value
            continue

        cpu_count = 0
        if int(value) != 0:
            cpu_count = int(value) * 0.01
        elif instance_id in coresDict:
            cpu_count = coresDict[instance_id]

        if cpu_count != 0:
            instanceDict[instance_id] = {
                'id': instance_id,
                'cores_num': cores_num,
                'cpu_count': cpu_count,
            }

    instances = []
    for key in instanceDict:
        instances.append(instanceDict[key])

    return instances


def get_host_instances(compute_node, user, coresDict):
    try:
        with common.ssh_client(compute_node, user=user, retries=1) as ssh:
            _, stdout, stderr = ssh.exec_command('cat /etc/yc/infra/nbs-throttling.json')
            if stderr.channel.recv_exit_status():
                print("ERROR: failed to cat nbs-throttling.json (%s)" % compute_node)
                return []

            data = json.loads(''.join(stdout.readlines()))
            cores_num = data.get('compute_cores_num')

            _, stdout, stderr = ssh.exec_command(
                'sudo yc-compute-node-keyctl list-endpoints | grep -e instance-id -e cpu_unit_count')
            if stderr.channel.recv_exit_status():
                print("ERROR: failed to get endpoint list (%s)" % compute_node)
                return []

            return parse_instances(cores_num, stdout.readlines(), coresDict)
    except:
        print("ERROR: failed to connect to %s" % compute_node)
        return []


def get_instances(compute_nodes, user, coresDict):
    futures = []
    with ThreadPoolExecutor(max_workers=32) as executor:
        for compute_node in compute_nodes:
            future = executor.submit(get_host_instances, compute_node, user, coresDict)
            futures.append(future)

    instances = []
    for future in futures:
        instances += future.result()

    with open('./instances.json', 'w') as outfile:
        json.dump(instances, outfile)

    return instances


def get_instances_from_file(file_path, coresDict):
    instances = []

    cores_num = 0
    n = 0

    lines = open(file_path).readlines()
    for i in range(len(lines)):
        line = lines[i].strip()
        if line.startswith("vla") or line.startswith("sas") or line.startswith("myt"):
            if i+2 < len(lines) and lines[i+1].strip() == "OUT[0]:":
                cores_num = int(lines[i+2].strip())
                n = i + 3

        line = line.strip()

        if line == "":
            if cores_num > 0:
                instances += parse_instances(cores_num, lines[n : i], coresDict)
            cores_num = 0

    return instances


def get_instance_factor(cluster, instance_id, cpu_count, cores_num, end):
    iops_per_quest = iops_per_host * cpu_count / cores_num
    bw_per_quest = bw_per_host * cpu_count / cores_num

    result = fetch_scalar(
        cluster,
        "server_volume",
        instance_id,
        "'Count|RequestBytes'",
        end,
        [("request", "'*'")])

    iopsArray = np.array([])
    bwArray = np.array([])
    for item in result:
        timeseries = item['timeseries']
        sens = timeseries['labels']['sensor']

        values = np.array(timeseries['values'])
        if values.size == 0:
            continue

        values[values == 'NaN'] = 0
        values = values.astype(np.float64)

        if sens == "Count":
            if iopsArray.size == 0:
                iopsArray = values
            else:
                iopsArray = np.add(iopsArray, values)

        if sens == "RequestBytes":
            if bwArray.size == 0:
                bwArray = values
            else:
                bwArray = np.add(bwArray, values)

    iops = 0
    if len(iopsArray) > 0:
        iops = np.percentile(iopsArray, 99.9, method='lower')

    bw = 0
    if len(bwArray) > 0:
        bw = np.percentile(bwArray, 99.9, method='lower')

    factor = iops/iops_per_quest + bw/bw_per_quest

    return {
        "instance_id": instance_id,
        "cpu_count": cpu_count,
        "cores_num": cores_num,
        "iops": iops,
        "bw": bw,
        "factor": factor,
    }


def get_instance_factor_s(cluster, instance_id, cpu_count, cores_num, end):
    try:
        return get_instance_factor(cluster, instance_id, cpu_count, cores_num, end)
    except:
        print("ERROR: failed to calculate factor for instance %s" % instance_id)
        return {}


def load_cores_dict(cores_file):
    coresDict = {}

    lines = open(cores_file).readlines()

    for i in range(len(lines)):
        if i == 0:
            continue
        line = lines[i]
        values = line.split("\t")
        if len(values) < 3:
            continue
        instance_id = values[0].strip()
        data = json.loads(values[1].strip())
        cores = data['cores']
        core_fraction = data.get('core_fraction', 100)
        coresDict[instance_id] = cores * core_fraction / 100.0

    return coresDict


def convert_bytes(size):
    for x in ['bytes', 'KB', 'MB', 'GB', 'TB']:
        if size < 1024.0:
            return "%3.1f %s" % (size, x)
        size /= 1024.0

    return "%3.1f %s" % (size, 'PB')


def print_top_factors(ycp_profile, logger, csv_file_path, factor_threshold):
    df = pd.read_csv(csv_file_path)
    df = df.reset_index()  # make sure indexes pair with number of rows

    ycp = Ycp(ycp_profile, logger, make_ycp_engine(False))

    top_df = df[df.factor > factor_threshold]

    for index, row in top_df.iterrows():
        try:
            instance = ycp.get_instance(row['instance_id'])
            cloud_id = ycp.get_cloud_id(instance.folder_id)
        except:
            cloud_id = "None"

        try:
            disks = [instance.boot_disk] + instance.secondary_disks
            sizes = [ycp.get_disk(disk).size for disk in disks]
            str = "[" + ', '.join([convert_bytes(int(size)) for size in sizes]) + "]"
        except:
            str = "[]"

        print(cloud_id, row['instance_id'], row['cpu_count'], "{:.3f}".format(row['factor']), str)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cluster", help="cluster [testing/preprod/prod_vla/prod_sas/prod_myt]", required=True)
    parser.add_argument("--user", help="ssh user", required=False)
    parser.add_argument("--compute-nodes", help="file with compute nodes", required=False)
    parser.add_argument("--instance-file", help="file with instance info", required=False)
    parser.add_argument("--cores-file", help="file with cores info", required=False)
    args = parser.parse_args()

    if args.cores_file is not None:
        coresDict = load_cores_dict(args.cores_file)
        print("cores dict size = %s" % len(coresDict))

    if args.compute_nodes is not None:
        compute_nodes = open(args.compute_nodes).read().split()

        print("compute node count = %s" % len(compute_nodes))
        instances = get_instances(compute_nodes, args.user, coresDict)
    elif args.instance_file is not None:
        instances = get_instances_from_file(args.instance_file, coresDict)
    else:
        print("set compute-nodes or instance-file arg")
        return

    print("instance count = %s" % len(instances))

    futures = []
    with ThreadPoolExecutor(max_workers=128) as executor:
        now = datetime.now()
        now = now.replace(minute=0, second=0)

        global tasks_total, tasks_completed
        tasks_total = len(instances)
        tasks_completed = 0

        for instance in instances:
            future = executor.submit(
                get_instance_factor_s,
                args.cluster,
                instance['id'],
                instance['cpu_count'],
                instance['cores_num'],
                now)
            future.add_done_callback(progress_indicator)
            futures.append(future)

    specs = []
    allFactors = np.array([])
    for future in futures:
        spec = future.result()
        if not spec:
            continue

        allFactors = np.append(allFactors, [spec['factor']])
        specs.append(spec)

    if allFactors.size > 0:
        overcommitFactor = np.percentile(allFactors, 99.9, method='lower')
        print("overcommit_factor=%s" % overcommitFactor)
    else:
        print("factor_count=%s" % allFactors.size)

    df = pd.DataFrame(specs)
    sorted_df = df.sort_values(by=['factor'], ascending=False)
    sorted_df.to_csv(f'./{args.cluster}_of.csv')

    # logger = common.create_logger("overcommit_factor", args)
    # print_top_factors(args.cluster, logger, f'./{args.cluster}_of.csv', 8.0)


main()
