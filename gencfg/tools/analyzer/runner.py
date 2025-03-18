#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import socket
import threading
import traceback
import time
from argparse import ArgumentParser

import api.cqueue
from kernel.util.errors import formatException

import functions
from instance_env import get_all_instances_env

import gencfg0
from gaux.aux_utils import getlogin


# from core.instances import Instance

class LocalHost(object):
    """Class, behaving like gencfg host (needed for sky)"""
    def __init__(self, host):
        self.name = host.name

    def __eq__(self, other):
        return self.name == other.name


class LocalInstance(object):
    """Class, behaving like gencfg instance (needed for sky)"""
    def __init__(self, instance):
        self.host = LocalHost(instance.host)
        self.port = instance.port
        self.power = instance.power
        self.type = instance.type
        self.N = instance.N

    def __hash__(self):
        return hash(self.host.name) ^ self.port

    def __eq__(self, other):
        return self.host == other.host and \
               self.port == other.port and \
               self.power == other.power and \
               self.type == other.type and \
               self.N == other.N

    def name(self):
        return '{}:{}'.format(self.host.name, self.port)


class CommonExecutor(object):
    def __init__(self, instances, functions, xparams, quiet=True):
        self.osUser = getlogin()

        self.instances = instances
        self.functions = functions
        self.quiet = quiet
        self.xparams = xparams

        self.result = {}
        self.exceptions = []

    def _run_instance(self, instance, xparams):
        for function in self.functions:
            try:
                self.result[instance][function.__name__] = True, function(
                    self.instances_env[(instance.host.name.split('.')[0], instance.port)], xparams)
            except Exception, e:
                self.result[instance][function.__name__] = False, traceback.format_exc(e)

    def run(self):
        self.instances_env = get_all_instances_env()
        # make short names
        self.instances_env = dict(
            map(lambda ((host, port), value): ((host.partition('.')[0], port), value), self.instances_env.iteritems()))

        #        my_host = socket.gethostname().split('.')[0]
        my_host = socket.gethostname()
        my_instances = filter(lambda x: x.host.name == my_host, self.instances)

        subs = []
        for instance in my_instances:
            self.result[instance] = {}
            if (instance.host.name.partition('.')[0], instance.port) not in self.instances_env:
                for function in self.functions:
                    self.result[instance][
                        function.__name__] = False, "Instance %s not found in instances_env" % instance.name()
            else:
                subs.append(threading.Thread(target=self._run_instance, args=(instance, self.xparams,)))
        for sub in subs:
            sub.start()
        for sub in subs:
            sub.join()

        return self.result


class Runner(object):
    def __init__(self, quiet=True):
        self.quiet = quiet

    def run_on_instances(self, instances, functions, timeout=400, xparams=None):
        if xparams is None:
            xparams = {}
        hosts = list(set(map(lambda x: x.host.name, instances)))

        if not self.quiet:
            print "%s: RUN on %d hosts with timeout %d" % (time.time(), len(hosts), timeout)

        serializable_instances = [LocalInstance(x) for x in instances]
        instances_by_host_port = { (x.host.name, x.port): x for x in instances }

        client = api.cqueue.Client('cqudp', netlibus=True)
        client.register_safe_unpickle('runner', 'LocalInstance')
        client.register_safe_unpickle('runner', 'LocalHost')
        client.register_safe_unpickle('tools.analyzer.runner', 'LocalInstance')
        client.register_safe_unpickle('tools.analyzer.runner', 'LocalHost')
        skynet_iterator = client.run(hosts, CommonExecutor(serializable_instances, functions, xparams, quiet=self.quiet)).wait(
            timeout)

        if not self.quiet:
            print "%s: STARTED iteration" % (time.time())

        failure_instances = []
        results = {}
        passed_hosts = []
        for host, result, failure in skynet_iterator:
            passed_hosts.append(host)
            if failure is not None:
                failure_instances.extend(filter(lambda x: x.host.name == host, instances))
                if not self.quiet:
                    print "Failure on %s" % host
                    print "Error message <%s>" % formatException(failure)
            else:
                for instance, instance_v in result.items():
                    instance = instances_by_host_port[(instance.host.name, instance.port)]
                    results[instance] = dict(
                        map(lambda (func, (succ, v)): (func, v if succ else None), instance_v.items()))

                    for func, (succ, v) in instance_v.items():
                        if not succ:
                            if not self.quiet:
                                print "Function %s failed on %s:%s" % (func, host, instance.port)
                                print "Error message <%s>" % v

        print "%s: FINISHED iteration (%d hosts done)" % (time.time(), len(passed_hosts))

        passed_hosts = set(passed_hosts)
        failure_hosts = []
        for host in hosts:
            if host not in passed_hosts:
                failure_hosts.append(host)
                if not self.quiet:
                    print "Timed out host " + host
        failure_hosts = set(failure_hosts)
        failure_instances.extend(filter(lambda x: x.host.name in failure_hosts, instances))

        for failure_instance in failure_instances:
            results[failure_instance] = {}

        return failure_instances, results

    def run_on_intlookups(self, intlookups, functions, run_base=True, run_int=False, timeout=400, xparams=None):
        if xparams is None:
            xparams = {}
        instances = []
        for intlookup in intlookups:
            if run_base:
                instances.extend(intlookup.get_used_base_instances())
            if run_int:
                instances.extend(intlookup.get_used_int_instances())

        print "%s: START ON %d instances" % (time.time(), len(instances))
        return self.run_on_instances(instances, functions, timeout=timeout, xparams=xparams)


def parse_cmd():
    import core.argparse.types as argparse_types

    parser = ArgumentParser("Calculated specified functions on specified instances")
    parser.add_argument("-i", "--instances", type=argparse_types.instances, required=True,
                        help="Obligatory. List of instances in format host:port")
    parser.add_argument("-u", "--functions", type=argparse_types.comma_list, required=True,
                        help="Optinal. List of functions/signals")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def normalize(options):
    options.functions = map(lambda x: functions.__dict__[x], options.functions)


def main(options):
    runner = Runner(quiet=True)
    failure_instances, result = runner.run_on_instances(options.instances, options.functions)

    print "Failured on %s" % ",".join(map(lambda x: x.name(), failure_instances))
    for instance in result:
        print "Instance %s" % instance.name()
        for signal in sorted(result[instance].keys()):
            print "    Signal %s: value <%s>" % (signal, result[instance][signal])


if __name__ == '__main__':
    options = parse_cmd()

    normalize(options)

    main(options)
