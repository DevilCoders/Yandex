import time
import re
import os
import socket
import urllib2

from gaux.aux_utils import run_command

from instance_env import EInstanceTypes

ISS_CGROUP_PATH = "/sys/fs/cgroup/freezer/slots/iss-agent"


# ==================================================================================================
# Auxliarily functions
# ==================================================================================================

def _host_ncpu():
    if os.uname()[0] == 'FreeBSD':
        retcode, out, err = run_command(["sysctl", "hw.ncpu"])
        return int(out.strip().split(' ')[1])
    elif os.uname()[0] == 'Linux':
        return int(os.sysconf('SC_NPROCESSORS_ONLN'))


def _detect_child_pids_recurse(pid):
    try:
        code, out, err = run_command(["pgrep", "-P", str(pid)])
    except:
        return []

    result = []
    child_pids = map(lambda x: int(x), out.split())
    for child_pid in child_pids:
        result.extend(_detect_child_pids_recurse(child_pid))
        result.append(child_pid)

    return result


def _detect_pids(instance_env):
    pids = []

    if instance_env["type"] == EInstanceTypes.BSCONFIG:
        piddir = os.path.join(instance_env["env"]["dir"], "pids")
        if os.path.exists(piddir):
            for fname in os.listdir(piddir):
                try:
                    with file(os.path.join(piddir, fname)) as f:
                        pids.append(int(f.read()))
                except:  # some files are empty, so just skip them
                    pass

        if ('a_itype_upper' in instance_env["env"]["tags"]) or ('a_itype_newsupper' in instance_env["env"]["tags"]):
            upperapache_pid_file = os.path.join(instance_env["env"]["index"]["dir"], "arkanavt", "upper_bundle",
                                                "httpd.pid")
            try:
                with file(upperapache_pid_file) as f:
                    pids.append(int(f.read()))
            except:
                pass

        if 'a_itype_vm' in instance_env["env"]["tags"]:
            try:
                retcode, out, err = run_command(["pidof", "kvm"])
                pids.append(int(out))
            except:
                pass

    elif instance_env["type"] == EInstanceTypes.ISS:
        pass

    pids = filter(lambda pid: os.path.exists("/proc/%s" % pid), pids)
    pids = pids + sum(map(lambda x: _detect_child_pids_recurse(x), pids), [])

    return pids


def _detect_cgroup(pid):
    try:
        with open(os.path.join("/proc", str(pid), "cgroup")) as f:
            data = f.read()
            slot = re.search(":memory:(.*)", data).group(1)
            if re.match('/slots/slot\d+', slot) or re.match('/slots/iss-agent', slot):
                return slot[1:]
    except:
        return None
    return None


def _detect_porto(env):
    if env['type'] == EInstanceTypes.BSCONFIG:
        envconf = '%s:%s' % (env['env']['conf'], env['env']['port'])
    elif env['type'] == EInstanceTypes.ISS:
        envconf = env['env']['containerPath']
    else:
        envconf = None

    status, out, err = run_command(["portoctl", "list"], timeout=60, raise_failed=False)
    if status != 0:
        return None
    avail_confs = map(lambda x: x.partition(' ')[0], out.split('\n'))

    for conf in avail_confs:
        if envconf == conf:
            return conf

    return None


# ===================================================================================================
# Host functions: return about the same result for all instances
# ===================================================================================================

def host_cpu_model(instance_env, xparams):
    del instance_env
    if os.uname()[0] == 'FreeBSD':
        retcode, model, _ = run_command(["sysctl", "-n", "hw.model"])
        model = model.strip()
    elif os.uname()[0] == 'Linux':
        model = re.search('model name([^\n]*)', open('/proc/cpuinfo').read()).group(0).partition(': ')[2]

    return xparams['known_cpu_models'][model]


def host_os(instance_env, xparams):
    del instance_env, xparams
    return os.uname()[0]


def host_cpu_usage(instance_env, xparams):
    del instance_env
    TIMEOUT = xparams.get('host_cpu_usage_timeout', 5)
    if os.uname()[0] == 'Linux':
        before = open('/proc/stat').readline()
        before = map(lambda x: int(x), re.search('cpu +(.*)', before).group(1).split(' '))

        time.sleep(TIMEOUT)

        after = open('/proc/stat').readline()
        after = map(lambda x: int(x), re.search('cpu +(.*)', after).group(1).split(' '))

        idle = after[3] - before[3]
        tot = sum(after) - sum(before)

        return (tot - idle) / float(tot)
    elif os.uname()[0] == 'FreeBSD':
        retcode, before, _ = run_command(["sysctl", "-n", "kern.cp_time"])
        beforet = time.time()

        time.sleep(TIMEOUT)

        retcode, after, _ = run_command(["sysctl", "-n", "kern.cp_time"])
        aftert = time.time()

        return (_host_ncpu() - (float(before.strip().split()[4]) - float(after.strip().split()[4])) / 133. / (
        aftert - beforet)) / _host_ncpu()
    else:
        raise Exception("Unkown os %s" % os.uname())


def host_ncpu(instance_env, xparams):
    del instance_env, xparams
    return _host_ncpu()


# ====================================================================================================
# Instance-specific functions. Some of them need waiting to measure "speed" (like instance_cpu_usage)
# ====================================================================================================

def _get_loadlog_datalines(fname, flt=lambda x: True):
    INTERVAL = 100.
    now = time.time()

    retcode, data, _ = run_command(["tail", "-10000", fname])

    datalines = data.rstrip().split('\n')
    if len(datalines) == 1 and datalines[0] == '':
        return None

    datalines = filter(lambda x: int(x.split('\t')[1]) > now - INTERVAL, datalines)
    datalines = filter(flt, datalines)
    if len(datalines) == 0:
        return None

    start = float(datalines[0].split('\t')[1]) + float(datalines[0].split('\t')[2]) / 1000000.
    return start, now, datalines


def instance_qps(instance_env, xparams):
    del xparams
    resp = _get_loadlog_datalines(instance_env["env"]["loadlog"])
    if not resp:
        return 0.0

    startt, endt, datalines = resp

    return len(datalines) / (endt - startt)


def instance_search_time_by_loadlog(instance_env, xparams):
    del xparams
    resp = _get_loadlog_datalines(instance_env["env"]["loadlog"])
    if not resp:
        return 0.0

    startt, endt, datalines = resp

    tottime = 0.0
    for line in datalines:
        if int(line.split('\t')[19]) == 1:
            tottime += int(line.split('\t')[8])

    return tottime / (endt - startt)


def instance_search_qps_by_loadlog(instance_env, xparams):
    del xparams
    resp = _get_loadlog_datalines(instance_env["env"]["loadlog"])
    if not resp:
        return 0.0

    startt, endt, datalines = resp

    qps = 0
    for line in datalines:
        if int(line.split('\t')[19]) == 1:
            qps += 1

    return float(qps) / (endt - startt)


def instance_factors_time_by_loadlog(instance_env, xparams):
    del xparams
    resp = _get_loadlog_datalines(instance_env["env"]["loadlog"])
    if not resp:
        return 0.0

    startt, endt, datalines = resp

    tottime = 0.0
    for line in datalines:
        if int(line.split('\t')[19]) == 4:
            tottime += int(line.split('\t')[8])

    return tottime / (endt - startt)


def instance_factors_qps_by_loadlog(instance_env, xparams):
    del xparams
    resp = _get_loadlog_datalines(instance_env["env"]["loadlog"])
    if not resp:
        return 0.0

    startt, endt, datalines = resp

    qps = 0
    for line in datalines:
        if int(line.split('\t')[19]) == 4:
            qps += 1

    return float(qps) / (endt - startt)


def instance_snippets_qps(instance_env, xparams):
    del xparams
    if instance_env["type"] != EInstanceTypes.BSCONFIG:
        return 0.0

    INTERVAL = 100.
    now = time.time()

    retcode, data, _ = run_command(["tail", "-10000", instance_env["env"]["loadlog"]])
    datalines = data.rstrip().split('\n')
    datalines = filter(lambda x: int(x.split('\t')[2]) / 1000000 > now - INTERVAL, datalines)
    if not len(datalines):
        return 0.

    start = float(datalines[0].split('\t')[2]) / 1000000.
    return len(datalines) / (now - start)


def instance_cpu_usage(instance_env, xparams):
    # get some system params
    HOST_OS = os.uname()[0]
    HZ = os.sysconf('SC_CLK_TCK')
    NCPU = _host_ncpu()
    TIMEOUT = xparams.get('instance_cpu_usage_timeout', 5)

    if instance_env["type"] != EInstanceTypes.FAKE:
        # detect pids
        pids = _detect_pids(instance_env)

        # meausure, wait timeout, measure again
        if HOST_OS == 'Linux':
            # check if process in porto
            porto_instance = _detect_porto(instance_env)
            if porto_instance is not None:
                status, out, err = run_command(["portoctl", "dget", porto_instance, "cpu_usage"])
                beforet = time.time()
                before = float(out) / 1000 / 1000 / 1000

                time.sleep(TIMEOUT)

                status, out, err = run_command(["portoctl", "dget", porto_instance, "cpu_usage"])
                aftert = time.time()
                after = float(out) / 1000 / 1000 / 1000

                return max(0., (after - before) / NCPU / (aftert - beforet))

            # go default stats
            def cpustat(pid):
                try:
                    return open(os.path.join('/proc', '%s' % pid, 'stat')).read().split()
                except:
                    return None

            before = map(cpustat, pids)
            beforet = time.time()

            time.sleep(TIMEOUT)

            after = map(cpustat, pids)
            aftert = time.time()

            result = 0.0
            for i in range(len(before)):
                if before[i] is not None and after[i] is not None:
                    result += (float(after[i][13]) - float(before[i][13]) + float(after[i][14]) - float(
                        before[i][14])) / (aftert - beforet) / HZ / NCPU
            return result
        elif HOST_OS == 'FreeBSD':
            def _convert(text):
                text = text.split('\n')[1]
                minutes, _, seconds = text.partition(':')
                seconds = re.sub(',', '.', seconds)  # Issue with russian locale
                return int(minutes) * 60 + float(seconds)

            beforet = time.time()
            before = map(lambda pid: run_command(["ps", "-o", "time", str(pid)])[1], pids)

            time.sleep(TIMEOUT)

            aftert = time.time()
            after = map(lambda pid: run_command(["ps", "-o", "time", str(pid)])[1], pids)

            return sum(map(lambda b, a: (_convert(a) - _convert(b)) / (aftert - beforet) / NCPU, before, after))
        else:
            raise Exception("Calculating instance cpu usage is not supported in os %s" % HOST_OS)
    else:  # fake instance cpu usage == all processes instance cpu usage
        if HOST_OS == 'Linux':
            before = open('/proc/stat').readline()
            before = map(lambda x: int(x), re.search('cpu +(.*)', before).group(1).split(' '))

            time.sleep(TIMEOUT)

            after = open('/proc/stat').readline()
            after = map(lambda x: int(x), re.search('cpu +(.*)', after).group(1).split(' '))

            idle = after[3] - before[3]
            tot = sum(after) - sum(before)

            return (tot - idle) / float(tot)
        elif HOST_OS == 'FreeBSD':
            status, before, _ = run_command(["sysctl", "-n", "kern.cp_time"])
            before = map(lambda x: int(x), before.split())

            time.sleep(TIMEOUT)

            status, after, _ = run_command(["sysctl", "-n", "kern.cp_time"])
            after = map(lambda x: int(x), after.split())

            idle = after[4] - before[4]
            tot = sum(after) - sum(before)
            return (tot - idle) / float(tot)
        else:
            raise Exception("Unkown os %s" % HOST_OS)


def instance_mem_usage(instance_env, xparams):
    del xparams
    CGROUP_MEM_ROOT = "/sys/fs/cgroup/memory"
    HOST_OS = os.uname()[0]

    if instance_env["type"] != EInstanceTypes.FAKE:
        pids = _detect_pids(instance_env)

        if HOST_OS == "Linux":
            # check if process in porto
            porto_instance = _detect_porto(instance_env)
            if porto_instance is not None:
                status, out, err = run_command(["portoctl", "dget", porto_instance, "memory_usage"])
                return float(out) / 1024 / 1024 / 1024

            # check if processes in cgroup
            cgroups = list(set(filter(lambda x: x is not None, map(_detect_cgroup, pids))))
            if len(cgroups) == 1:  # found exactly one cgroup
                return float(open(
                    os.path.join(CGROUP_MEM_ROOT, cgroups[0], "memory.usage_in_bytes")).read()) / 1024 / 1024 / 1024

            # go default stats
            if 'a_ctype_hamster' in instance_env["env"]["tags"]:
                # hamster should be treated separately
                def memstat(pid):
                    try:
                        fname = os.path.join('/proc', '%s' % pid, 'statm')
                        with file(fname) as f:
                            data = f.read()

                        pages = int(data.split()[1]) - int(data.split()[2])

                        return float(pages) * 4096 / 1024 / 1024 / 1024
                    except:
                        raise

                return sum(map(lambda x: memstat(x), pids))

            else:
                # we can not calculate memory usage precisely without cgroup so suppose all VmLib memory
                # is common for all instances
                def memstat(pid):
                    try:
                        fname = os.path.join('/proc', '%s' % pid, 'status')
                        with file(fname) as f:
                            data = f.read()

                        rss = float(re.search("VmRSS:\s+(\d+)", data).group(1)) / 1024 / 1024
                        lib = float(re.search("VmLib:\s+(\d+)", data).group(1)) / 1024 / 1024

                        return rss, lib
                    except:
                        return 0., 0.

                rr = map(lambda x: memstat(x), pids)

                if len(rr):
                    total_rss = sum(map(lambda x: x[0], rr))
                    total_lib = sum(map(lambda x: min(x[0], x[1]), rr))
                    max_lib = max(map(lambda x: x[1], rr))
                    return total_rss - total_lib + max_lib
                else:
                    return 0.

        elif HOST_OS == "FreeBSD":
            if len(pids) == 0:
                return 0.0

            status, out, err = run_command(["ps", "-o", "rss", "-p", ','.join(map(lambda x: str(x), pids))])

            total = 0
            for line in out.split("\n"):
                try:
                    total += int(line)
                except ValueError:
                    pass

            return float(total) / 1024 / 1024
        else:
            raise Exception("Calculating instance mem usage is not supported in os %s" % HOST_OS)
    else:  # fake instance memory usage == all memory - free memory
        if HOST_OS == "Linux":
            with open("/proc/meminfo") as f:
                data = f.read()
                return (float(re.search("MemTotal:\s+(\d+)", data).group(1)) - float(
                    re.search("MemFree:\s+(\d+)", data).group(1))) / 1024 / 1024
        elif HOST_OS == "FreeBSD":
            return 0.  # FIXME: not implemented
        else:
            raise Exception("Calculating instance mem usage is not supported in os %s" % HOST_OS)


def instance_is_running(instance_env, xparams):
    del xparams
    try:
        retcode, data, _ = run_command(["nc", "-z", socket.gethostname(), instance_env["env"]["port"]])
        return 1
    except:
        return 0


def _instance_time_median(instance_env, ratio):
    retcode, data, _ = run_command(["tail", "-10000", instance_env["env"]["loadlog"]], 50)
    data = data.rstrip().split('\n')
    if len(data) == 1:
        return 0.

    timings = map(lambda x: int(x.split('\t')[0]), data)
    timings = sorted(filter(lambda x: x > 0, timings))
    l = len(timings)

    return timings[int(l * ratio)]


def instance_time_median50(instance_env, xparams):
    del xparams
    return _instance_time_median(instance_env, 0.5)


def instance_time_median90(instance_env, xparams):
    del xparams
    return _instance_time_median(instance_env, 0.9)


def instance_time_median95(instance_env, xparams):
    del xparams
    return _instance_time_median(instance_env, 0.95)


def instance_time_median99(instance_env, xparams):
    del xparams
    return _instance_time_median(instance_env, 0.99)


def instance_unanswer_rate(instance_env, xparams):
    del xparams
    retcode, data, _ = run_command(["tail", "-10000", instance_env["env"]["loadlog"]])
    data = data.rstrip().split('\n')
    if len(data) == 1:
        return 0.

    unanswer_count = len(filter(lambda x: x < 0, map(lambda x: int(x.split('\t')[0]), data)))
    all_count = len(data)

    return unanswer_count / float(all_count)


def instance_documents(instance_env, xparams):
    del xparams
    if instance_env["type"] == EInstanceTypes.BSCONFIG:
        fname = os.path.join(instance_env["indexdir"], 'indexprn')
        return os.stat(fname).st_size / 2
    else:
        return 0


def instance_database_size(instance_env, xparams):
    del xparams
    if instance_env["type"] == EInstanceTypes.BSCONFIG:
        path = instance_env["env"]["indexdir"]
    else:
        path = None

    if path is None:
        return 0

    total_size = 0
    seen = set()
    for dirpath, dirnames, filenames in os.walk(path):
        for f in filenames:
            fp = os.path.join(dirpath, f)
            if os.path.islink(fp):
                continue
            try:
                stat = os.stat(fp)
            except OSError:
                continue
            if stat.st_ino in seen:
                continue
            seen.add(stat.st_ino)
            total_size += stat.st_size

    return total_size


def instance_major_tag(instance_env, xparams):
    del xparams
    return instance_env["env"].get("major_tag", 0)


def instance_minor_tag(instance_env, xparams):
    del xparams
    return instance_env["env"].get("minor_tag", 0)


def instance_version(instance_env, xparams):
    del xparams
    return instance_env["env"].get("version", 0)


def instance_group(instance_env, xparams):
    del xparams
    return instance_env["env"].get("group", "")


def instance_supermind_mode(instance_env, xparams):
    del xparams
    url = "http://127.0.0.1:%s/supermind?action=getmode" % (instance_env['env']['port'])
    try:
        return urllib2.urlopen(url, timeout=10).read().strip()
    except:
        return "unknown"


def instance_supermind_mult(instance_env, xparams):
    del xparams
    url = "http://127.0.0.1:%s/supermind?action=getmult" % (instance_env['env']['port'])
    try:
        return int(urllib2.urlopen(url, timeout=10).read().strip())
    except:
        return 1.0


def instance_tags(instance_env, xparams):
    del xparams
    return instance_env["env"].get("tags", [])


def dummy(instance_env, xparams):
    del instance_env, xparams
    return 0
