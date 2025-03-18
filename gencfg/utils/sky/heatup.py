#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import pwd
import xmlrpclib
import socket
import subprocess
import time
from optparse import OptionParser
import fcntl
import signal

import api.cqueue
import api.copier
from kernel.util.errors import formatException

import gencfg0

# FIXME: change this thing as soon as possible
# Skynet uses msgpack of version 0.2.0, while gencfg uses 0.4.6 . Importing gencfg0 changes path to <import msgpack> from skynet to gencfg one.
# As a result, skynet tries to pack network data using gencfg msgpack and can non unpack using version 0.2.0 on remote host
if sys.path[0] == gencfg0._packages_path:
    sys.path.pop(0)
    sys.path.append(gencfg0._packages_path)

from config import MAIN_DIR

STEPS = 20
VERBOSE_LEVEL = 0
SANDBOX_URL = 'https://sandbox.yandex-team.ru/sandbox/xmlrpc'

TIERS_PLANS = {
    'OxygenExpPlatinumTier0': 'PlatinumTier0_priemka',
    'OxygenExpWebTier0': 'RusTier0_priemka',
    'OxygenExpWebTier1': 'RusTier0_priemka',
    'OxygenExpRRGTier0': 'RusTier0_priemka',
    'OxygenExpEngTier0': 'EngTier0_priemka',
    'OxygenExpEngTier1': 'EngTier0_priemka',
    'OxygenExpDivTier0': 'RusTier0_priemka',
    'OxygenExpDivUa0': 'DiversityUa_priemka',
    'OxygenExpPlatinumTurTier0': 'TurTier0_priemka_ams',
    'OxygenExpTurTier0': 'TurTier0_priemka_ams',
    'OxygenExpTurTier1': 'TurTier0_priemka_ams',
    'OxygenExpItaTier0': 'RusTier0_priemka',  # FIXME
    'PlatinumTier0': 'PlatinumTier0_priemka',
    'WalrusPlatinumTier0': 'PlatinumTier0_priemka',
    'RRGTier0': 'RusTier0_priemka',
    'EngTier0': 'EngTier0_priemka',
    'EngTier1': 'EngTier0_priemka',
    'Div0': 'RusTier0_priemka',
    'DivUa0': 'DiversityUa_priemka',
    'PlatinumTurTier0': 'TurTier0_priemka_ams',
    'TurTier0': 'TurTier0_priemka_ams',
    'TurTier1': 'TurTier0_priemka_ams',
    'ItaTier0': 'RusTier0_priemka',  # FIXME
    'ImgTier0': 'images_heatup_base_main',
    'ImgQuickTier0': 'images_heatup_base_quick',
    'VideoTier0': 'video_heatup_base_main',
    'WebTier0': 'RusTier0_priemka',
    'WalrusWebTier0': 'RusTier0_priemka',
    'WebTier1': 'RusTier0_priemka',
    'KzTier0': 'PlatinumTier0_priemka',
}

MSK_CONFIGS = [
    'msk.web.yaml',
]

SAS_CONFIGS = [
    'sas.web.yaml',
]

MAN_CONFIGS = [
    'man.web.yaml',
]

PIP_INTLOOKUPS = [
    'intlookup-msk-p-in-production.py',
    'intlookup-msk-rrg-p-in-production.py',
]


def _load_intlookup_from_sas_configs(fnames):
    import core.argparse.types as argparse_types

    result = []
    for fname in fnames:
        sas_config = argparse_types.sasconfig(os.path.join(MAIN_DIR, 'optimizers', 'sas', 'configs', fname))
        result.extend(map(lambda x: x.intlookup, sas_config))
    return result


class Dolbilo(object):
    def __init__(self, heatupData, freebsdDolbilo, linuxDolbilo, user, N, threads, max_host_usage, timeout, tmpdir=None,
                 dryrun=False):
        self.heatupData = heatupData
        self.freebsdDolbilo = freebsdDolbilo
        self.linuxDolbilo = linuxDolbilo
        self.osUser = user
        self.N = N
        self.threads = threads
        self.max_host_usage = max_host_usage
        self.timeout = timeout
        self.dryrun = dryrun

        if tmpdir is None:
            self.tmpdir = os.path.join('/var/tmp', 'heatup_%s' % self.osUser)
        else:
            self.tmpdir = os.path.join('/var/tmp', tmpdir)

    def _prepare(self):

        try:
            os.mkdir(self.tmpdir)
        except:
            pass
        try:
            os.chmod(self.tmpdir, 0777)
        except:
            pass

        copier = api.copier.Copier()
        fetch_resource(copier, self.freebsdDolbilo, os.path.join(self.tmpdir, 'freebsd', 'd-executor'))
        fetch_resource(copier, self.linuxDolbilo, os.path.join(self.tmpdir, 'linux', 'd-executor'))

        unique_plans = set(map(lambda (x, y): y, self.myHeatupData))

        for plan in unique_plans:
            fetch_resource(copier, plan, os.path.join(self.tmpdir, plan, "RESOURCE"))

    def _run(self):
        if os.uname()[0] == "Linux":
            prefix = "linux"
            if self.max_host_usage != 0:
                max_used_cores = int(int(os.sysconf('SC_NPROCESSORS_ONLN')) * self.max_host_usage / 100)
            else:
                max_used_cores = sys.maxint
        elif os.uname()[0] == "FreeBSD":
            prefix = "freebsd"
            max_used_cores = sys.maxint
        else:
            raise Exception("Unknown os %s" % os.uname()[0])

        if self.threads > max_used_cores:
            raise Exception("Can use only %d cores, while one basesearch will use %d" % (max_used_cores, self.thread))

        wait_till = time.time() + self.timeout + 5  # give d-executor some time to finish on `--time-limit'

        unstarted = []
        subs = []

        try:
            for (host, port), plan in self.myHeatupData:
                args = {
                    '_lockfile': os.path.join(self.tmpdir, 'test-%s.lock' % port),
                    'args': (os.path.join(self.tmpdir, prefix, 'd-executor'),
                             '--plan-file', os.path.join(self.tmpdir, plan, 'RESOURCE'),
                             '--output', os.path.join(self.tmpdir, 'test-%s.out' % port),
                             '--mode', 'finger', '--simultaneous', '%s' % self.threads,
                             '--replace-host', 'localhost', '--replace-port', str(port),
                             '--queries-limit', str(self.N),
                             '--time-limit', str(self.timeout),
                             '--circular'),
                    'stdout': os.path.join(self.tmpdir, 'test-%s.stdouterr' % port),
                    'stderr': subprocess.STDOUT,
                }
                #                if os.path.exists('/sys/fs/cgroup/memory/production'): # Linux with cgroups
                #                    args = ['cgexec', '-g', 'cpu,blkio,memory,freezer:/production', '--sticky'] + args
                unstarted.append(args)

            while time.time() < wait_till:
                while (len(subs) + 1) * self.threads < max_used_cores and len(unstarted) > 0:
                    for undx, u in enumerate(unstarted[:]):
                        with open(u['_lockfile'], 'w') as fd:
                            try:
                                fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
                            except IOError:
                                continue  # try next `unstarted`
                            u = unstarted.pop(undx)
                            del u['_lockfile']
                            u['stdout'] = open(u['stdout'], 'w')  # do not truncate file till lock is taken
                            subs.append(subprocess.Popen(**u))  # subprocess inherits the lock
                            break  # from `for`
                    else:
                        break  # from `while`

                for p in subs:
                    p.poll()
                    if p.returncode:
                        raise Exception("Command %s exited with code %s" % (args, p.returncode))

                subs = filter(lambda x: x.returncode is None, subs)
                if len(subs) == 0 and len(unstarted) == 0:
                    return
                time.sleep(1)
        finally:
            for p in subs:
                try:
                    p.kill()
                except:
                    pass

            sub = subprocess.Popen(['pgrep', 'd-executor'], stdout=subprocess.PIPE, stderr=None)
            pids = sub.communicate()[0]
            for pid in pids.strip().split('\n'):
                try:
                    if pid:
                        os.kill(int(pid), signal.SIGKILL)
                except Exception:
                    pass

    def run(self):
        if self.dryrun:
            return 0.0

        startt = time.time()

        self.myHost = socket.gethostname().split('.')[0]
        self.myHeatupData = filter(lambda ((host, port), plan): host.split('.')[0] == self.myHost, self.heatupData)
        self._prepare()
        self._run()

        endt = time.time()

        return endt - startt


def fetch_resource(copier, src_path, dest_path):
    """
        Utility method to retrive files from skynet and store
        them with specified destination filename
    """

    dest_dir = os.path.dirname(dest_path)

    resource_files = copier.handle(src_path).list().wait().files()
    resource_names = [
        resource["name"] for resource in resource_files if resource["type"] == "file"
        ]
    if len(resource_names) != 1:
        raise Exception("Wrong skynet resource: %s" % src_path)

    copier.get(src_path, dest_dir).wait()

    resource_path = os.path.join(dest_dir, resource_names[0])
    if resource_path == dest_path:
        return

    if os.path.exists(dest_path):
        os.remove(dest_path)
    os.symlink(resource_path, dest_path)


def parse_cmd():
    WEB_LOCATIONS = ["msk", "sas", "man", "pip"]
    SERVICES = ["web", "img-main", "img-quick", "video-main", "oxygen"]

    usage = "Usage %prog [options]"
    parser = OptionParser(usage=usage)

    parser.add_option("-q", "--queries", type=int, dest="queries",
                      help="Obligatory. Number of queries")
    parser.add_option("-l", "--location", dest="location", choices=WEB_LOCATIONS, default="msk",
                      help="Obligatory. Geo location: %s" % ", ".join(WEB_LOCATIONS))
    parser.add_option("-S", "--service", dest="service", choices=SERVICES, default="web",
                      help="Optional. Service type: %s" % ", ".join(SERVICES))
    parser.add_option("-i", "--intlookups", type=str, dest="intlookups", default=None,
                      help="Optional. Comma-separated list of <intlookup:sandbox_tag> elements. Overrides --location and --service options")
    parser.add_option("-t", "--timeout", type="int", default=100000,
                      help="Optinal. Maximal wait time")
    parser.add_option("-p", "--success-percent", type="float", dest="success_percent", default=0.0,
                      help="Optional. Success percent (range [0.0, 1.0])")
    parser.add_option("-r", "--threads", type="int", dest="threads", default=6,
                      help="Optional. Number of threads per executor")
    parser.add_option("--max-host-usage", type=float, dest="max_host_usage", default=0,
                      help="Optional. Maximum host usage during heatup  in percents [0, 100] (0 to disable this feature)")
    parser.add_option("-m", "--tmpdir", type="str", dest="tmpdir", default=None,
                      help="Optional. Tmp dir postfix")
    parser.add_option("-f", "--filtered", type="str", dest="filtered", default=None,
                      help="Optional. Filter hosts by skynet groups (e. g. '+SEARCH2 +SEARCH20 +ws30-300')")
    parser.add_option("-u", "--user", type="str", dest="user", default=pwd.getpwuid(os.getuid())[0],
                      help="Optional. User name")
    parser.add_option("-k", "--key-file", type="str", dest="key_file", default=None,
                      help="Optional. Custom ssh private key file")
    parser.add_option("--dryrun", action="store_true", default=False,
                      help="Optional. Make dry run")

    def increase_verbose(*args):
        global VERBOSE_LEVEL
        VERBOSE_LEVEL += 1

    parser.add_option("-v", "--verbose", action="callback", callback=increase_verbose,
                      help="Optional. Verbose mode. Multiple -v options increase the verbosity.  The maximum is 3.")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    (options, args) = parser.parse_args()

    if options.queries is None:
        raise Exception("Option --queries is obligatory")
    if options.success_percent > 1.0 or options.success_percent < 0.0:
        raise Exception("Success percent must be in [0.0, 1.0] range")

    options.verbose_level = VERBOSE_LEVEL
    if options.intlookups:
        options.intlookups = dict(map(lambda x: (x.split(':')[0], x.split(':')[1]), options.intlookups.split(',')))
    if options.filtered:
        from library.sky.hosts import resolveHosts
        options.filtered = map(lambda x: x, resolveHosts([options.filtered])[0])
        print options.filtered

    return options


if __name__ == '__main__':
    options = parse_cmd()

    if options.verbose_level >= 1:
        print "[%s] Started as %s" % (int(time.time()), ' '.join(sys.argv))

    if options.intlookups is not None:
        INTLOOKUPS = options.intlookups
    elif options.service == "web":
        if options.location == "msk":
            INTLOOKUPS = _load_intlookup_from_sas_configs(MSK_CONFIGS)
        if options.location == "sas":
            INTLOOKUPS = _load_intlookup_from_sas_configs(SAS_CONFIGS)
        if options.location == "man":
            INTLOOKUPS = _load_intlookup_from_sas_configs(MAN_CONFIGS)
        if options.location == "pip":
            INTLOOKUPS = PIP_INTLOOKUPS
    elif options.service in ("img-main", "img-quick"):
        index_type = options.service.split("-", 1)[1]
        if options.location not in ("msk", "sas", "man"):
            raise Exception("Unsupported location for images service")
        INTLOOKUPS = ["%s_IMGS_BASE%s" % (options.location.upper(), "" if index_type == "main" else "_QUICK")]
    elif options.service == "video-main":
        if options.location not in ("msk", "sas", "man"):
            raise Exception("Unsupported location for video service")
        INTLOOKUPS = ["%s_VIDEO_BASE" % options.location.upper()]
    elif options.service == "oxygen":
        INTLOOKUPS = ["intlookup-msk-oxygen-exp.py"]

    proxy = xmlrpclib.ServerProxy(SANDBOX_URL)

    # prepare data
    heatupData = []
    for fname in INTLOOKUPS:
        if options.verbose_level >= 1:
            print "Preparing %s" % fname

        from core.db import CURDB

        intlookup = CURDB.intlookups.get_intlookup(fname)


        def _resource_skynet_id(tier):
            if tier not in TIERS_PLANS:
                raise Exception("Not found plan for tier %s" % tier)

            avail_plans = proxy.listResources({
                'resource_type': 'BASESEARCH_PLAN',
                'state': 'READY',
                'attr_name': TIERS_PLANS[tier],
                'attr_value': '1',
                'limit': 1,
            })

            if len(avail_plans) == 0:
                raise Exception("Could not find doliblo plan: resource_type <BASEARCH_PLAN> , plan attribute name <%s> , plan attribute value <%s>. All plans might be removed from sandbox due to expiration or other reasons" % (TIERS_PLANS[tier], '1'))

            return avail_plans[0]['skynet_id']


        plans = dict(map(lambda x: (x, _resource_skynet_id(x)), intlookup.tiers))
        for instances, tier in intlookup.get_used_instances_with_tier():
            for instance in instances:
                heatupData.append(((instance.host.name, instance.port), plans[tier]))

    freebsdDolbilo = \
    proxy.listResources({'resource_type': 'DEXECUTOR_EXECUTABLE', 'state': 'READY', 'limit': '1', 'arch': 'freebsd'})[
        0]['skynet_id']
    linuxDolbilo = \
    proxy.listResources({'resource_type': 'DEXECUTOR_EXECUTABLE', 'state': 'READY', 'limit': '1', 'arch': 'linux'})[0][
        'skynet_id']

    if options.verbose_level >= 1:
        print "Prepared plans (skynet ids): %s" % " ".join(
            sorted(list(set(plans.values()))) + [freebsdDolbilo, linuxDolbilo])

    client = api.cqueue.Client('cqudp', netlibus=True)

    if options.key_file:
        key = client.signer.loadKeys(open(options.key_file, 'rb'), options.key_file).next()
        client.signer.addKey(key)

    hosts = list(set(map(lambda ((host, port), plan): host, heatupData)))
    if options.filtered:
        if options.dryrun:
            hosts = options.filtered
        else:
            hosts = list(set(hosts) & set(options.filtered))

    user = pwd.getpwuid(os.getuid())[0]
    iterator = client.run(hosts, Dolbilo(heatupData, freebsdDolbilo, linuxDolbilo, options.user, options.queries,
                                         options.threads, options.max_host_usage, options.timeout,
                                         tmpdir=options.tmpdir, dryrun=options.dryrun))

    if options.verbose_level >= 1:
        print "Running on %s hosts" % len(hosts)
    failed_hosts, success_hosts = 0, 0
    processed_hosts = set()
    step = 0
    for host, result, failure in iterator.wait(options.timeout):
        if host in processed_hosts:  # got 2nd result from single host, ignore it
            continue
        processed_hosts.add(host)

        if failure is not None:
            if options.verbose_level >= 3:
                print formatException(failure)
            failed_hosts += 1
        else:
            success_hosts += 1

        if options.verbose_level >= 2:
            print "Success %s, Failed %s (last %s %s, time %s)" % (
            success_hosts, failed_hosts, host, "success" if failure is None else "failure", result)
            sys.stdout.flush()
        if options.verbose_level >= 1:
            if (success_hosts + failed_hosts) == len(hosts) * (step + 1) / STEPS:
                print "%s: %s%% Done (Success %s, Failed %s, Left %s)" % (
                time.strftime("[%Y-%m-%d %H:%M:%S]", time.localtime()),
                (success_hosts + failed_hosts) * 100 / len(hosts), success_hosts, failed_hosts,
                len(hosts) - success_hosts - failed_hosts)
                sys.stdout.flush()
                step += 1

    if options.verbose_level >= 1:
        print "Finished (Success %s, Failed %s)" % (success_hosts, len(hosts) - success_hosts)
        print "    Timeout hosts: %s" % (",".join(set(hosts) - set(processed_hosts)))

    if success_hosts / float(len(hosts)) > options.success_percent:
        sys.exit(0)
    else:
        sys.stderr.write("Heatup failed: Success %s, Failed %s, Needed %s\n" % (
        success_hosts, failed_hosts, len(hosts) * options.success_percent))
        sys.exit(1)
