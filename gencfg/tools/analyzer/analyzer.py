#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import pickle
import sqlite3
from collections import defaultdict, OrderedDict
from argparse import ArgumentParser

import gencfg0

import core.argparse.types as argparse_types
from gaux.aux_colortext import red_text
from gaux.aux_utils import get_corrected_power_usage

from runner import Runner
import functions


class Analyzer(object):
    def __init__(self, region_map_file=None):
        self.data = {}
        self.indexes = {}

        if region_map_file is not None:
            self.region_map = dict(map(lambda (tier, shard_id, region): ((tier, int(shard_id)), region),
                                       map(lambda x: x.strip().split(), open(region_map_file).readlines())))
        else:
            self.region_map = None

    def _add_instance_data(self, instances_data, intlookup, instance, shard_id):
        if instance not in instances_data:
            return

        instances_data[instance]['shard_id'] = shard_id
        instances_data[instance]['tier'] = 'None' if intlookup.tiers is None else intlookup.get_tier_for_shard(shard_id)
        instances_data[instance]['host'] = instance.host.name
        instances_data[instance]['switch'] = instance.host.switch
        instances_data[instance]['queue'] = instance.host.queue
        instances_data[instance]['dc'] = instance.host.dc
        instances_data[instance]['port'] = instance.port
        instances_data[instance]['config_power'] = instance.power
        instances_data[instance]['type'] = instance.type
        instances_data[instance]['intlookup_name'] = intlookup.file_name
        if self.region_map is not None:
            instances_data[instance]['region'] = self.region_map.get((intlookup.get_tier_for_shard(shard_id)), None)

    def _add_internal_data(self, instances_data, intlookups):
        for intlookup in intlookups:
            for i in range(intlookup.brigade_groups_count * intlookup.hosts_per_group):
                for basesearch in intlookup.get_base_instances_for_shard(i):
                    self._add_instance_data(instances_data, intlookup, basesearch, i)
            for i in range(0, intlookup.brigade_groups_count * intlookup.hosts_per_group, intlookup.hosts_per_group):
                for intsearch in intlookup.get_int_instances_for_shard(i):
                    self._add_instance_data(instances_data, intlookup, intsearch, i)

    def _add_calculated_data(self, instances_data):
        for instance in instances_data:
            idict = instances_data[instance]

            iusage = idict.get('instance_cpu_usage', None)
            husage = idict.get('host_cpu_usage', None)
            if iusage and husage is not None:
                model = CURDB.cpumodels.get_model(instance.host.model)
                idict['used_power'] = get_corrected_power_usage(model, iusage, husage)

    def create_indexes(self):
        # find all keys
        keys = set()
        for instance in self.data:
            for k in self.data[instance]:
                keys.add(k)
        for k in keys:
            self.indexes[k] = defaultdict(set)

        for instance in self.data:
            for k, v in self.data[instance].items():
                self.indexes[k][v].add(instance)

    def show_filtered(self, show_fields=None, filters=None, orderby=None):
        # apply filters
        if orderby is None:
            orderby = []
        if filters is None:
            filters = []
        if show_fields is None:
            show_fields = []
        filtered_instances = self.data.keys()
        for name, value in filters:
            filtered_instances = filter(lambda x: x in self.indexes[name][value], filtered_instances)

        # apply orderby
        def sort_func(i1, i2):
            for param in orderby:
                if self.data[i1][param] < self.data[i2][param]:
                    return -1
                if self.data[i1][param] > self.data[i2][param]:
                    return 1

            return cmp("%s:%s" % (i1.name, i1.port), "%s:%s" % (i2.name, i2.port))

        filtered_instances.sort(cmp=sort_func)

        # show
        for instance in filtered_instances:
            print "%s:%s >> %s" % (instance.name, instance.port,
                                   ', '.join(map(lambda x: "%s : %s" % (x, self.data[instance][x]), show_fields)))

    def load_from_file(self, fname):
        f = open(fname, 'r')
        self.data = pickle.load(f)
        f.close()

    def save_to_file(self, fname):
        f = open(fname, 'w')
        pickle.dump(self.data, f, -1)
        f.close()

    def save_to_sqlite(self, fname, tablename):
        # create new connection
        conn = sqlite3.connect(fname)
        c = conn.cursor()

        # find all keys
        keys = OrderedDict()
        keys['sid'] = 'text'
        keys['host'] = 'text'
        keys['port'] = 'integer'
        for instance in self.data:
            #            print self.data[instance]
            for k in self.data[instance]:
                if self.data[instance][k] is None:
                    continue
                if isinstance(self.data[instance][k], (int, long)):
                    keys[k] = 'integer'
                elif isinstance(self.data[instance][k], float):
                    keys[k] = 'real'
                elif isinstance(self.data[instance][k], str):
                    keys[k] = 'text'
                elif isinstance(self.data[instance][k], tuple):
                    keys[k] = 'text'
                elif isinstance(self.data[instance][k], list):
                    keys[k] = 'text'
                    self.data[instance][k] = ','.join(self.data[instance][k])
                else:
                    print red_text("Unknown type <%s> in result (instance <%s> key <%s>, skipping..." % (self.data[instance][k].__class__, instance, k))
        keys = keys.items()

        # create table
        cnt = c.execute("SELECT count(*) FROM sqlite_master WHERE type = 'table' AND name = ?",
                        (options.table_name,)).fetchone()[0]
        if cnt == 0:
            c.execute('CREATE TABLE %s (%s)' % (tablename, ', '.join(map(lambda (x, y): '%s %s' % (x, y), keys))))

        # fill data
        for instance in self.data:
            values = []
            for k in keys:
                value = self.data[instance].get(k[0], None)
                if value is None:
                    values.append("NULL")
                else:
                    values.append("'%s'" % value)
            c.execute('INSERT INTO %s VALUES (%s)' % (tablename, ', '.join(values)))
        conn.commit()

        print "Added %d records to db" % len(self.data)

    def _add_default_xparams(self, xparams):
        xparams['known_cpu_models'] = dict(map(lambda x: (x.fullname, x.model), CURDB.cpumodels.models.values()))

    def load_from_runner(self, intlookups, run_instances=None, run_functions=None, quiet=True, timeout=100,
                         run_int=False,
                         xparams=None):
        if xparams is None:
            xparams = {}
        self._add_default_xparams(xparams)

        r = Runner(quiet=quiet)
        if run_functions is None:
            run_functions = [functions.host_ncpu, functions.host_cpu_model, functions.instance_qps,
                             functions.instance_cpu_usage, functions.host_os, functions.instance_is_running,
                             functions.instance_time_median50, functions.instance_time_median90,
                             functions.instance_time_median95, functions.instance_time_median99,
                             functions.instance_unanswer_rate, functions.host_cpu_usage]

        if run_instances is None:
            failure_instances, instances_data = r.run_on_intlookups(intlookups, run_functions, run_base=True,
                                                                    run_int=run_int, timeout=timeout, xparams=xparams)
        else:
            failure_instances, instances_data = r.run_on_instances(run_instances, run_functions, timeout=timeout,
                                                                   xparams=xparams)

        print "Failure on %d instances" % len(failure_instances)

        if intlookups:
            self._add_internal_data(instances_data, intlookups)
        else:
            for instance in instances_data:
                instances_data[instance]['host'] = instance.host.name
                instances_data[instance]['port'] = instance.port
                instances_data[instance]['sid'] = options.sid

        self._add_calculated_data(instances_data)

        for instance in instances_data:
            self.data[instance] = instances_data[instance]


def parse_cmd():
    parser = ArgumentParser(description="Get instances various usage statistics (for futher analysis)")
    parser.add_argument("-f", "--out-file", type=str, dest="out_file", default=None, required=True,
                        help="Obligatory. Output db file")
    parser.add_argument("--table-name", type=str, dest="table_name", default="data",
                        help="Optional. Table name in db file")
    parser.add_argument("--sid", type=str, default="default",
                        help="Optional. Identifier of run in result table")
    parser.add_argument("-i", "--intlookups", type=argparse_types.intlookups, dest="intlookups", default=None,
                        help="Optional. Intlookups to analyze")
    parser.add_argument("-s", "--sas-configs", type=argparse_types.sasconfigs, dest="sas_configs", default=None,
                        help="Optional. Sas config to get list of intlookups")
    parser.add_argument("-t", "--timeout", type=int, dest="timeout", default=100,
                        help="Optional. Skynet timoout")
    parser.add_argument("-o", "--hosts", type=argparse_types.hosts, dest="hosts", default=None,
                        help="Optional. Hosts to analyze")
    parser.add_argument("-u", "--functions", type=str, dest="functions", default=None,
                        help="Optinal. List of functions/signals")
    parser.add_argument("-q", "--quiet", action="store_true", dest="quiet", default=False,
                        help="Optional. Rethrow exceptions")
    parser.add_argument("-r", "--region-file", type=str, dest="region_file", default=None,
                        help="Optional. File with region mapping")
    parser.add_argument("-x", "--xparams", type=str, dest="xparams", default=None,
                        help="Optional. Extra params for functions as python dict")
    parser.add_argument("--run-int", action="store_true", dest="run_int", default=False,
                        help="Optional. Get statistics from ints also")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if int(options.intlookups is not None) + int(options.sas_configs is not None) + int(options.hosts is not None) != 1:
        raise Exception("You must specify exactly on of --intlookups --sas-config option")

    if options.sas_configs:
        options.intlookups = map(
            lambda sasconfig: map(lambda x: CURDB.intlookups.get_intlookup(x.intlookup), sasconfig),
            options.sas_configs)
        options.intlookups = sum(options.intlookups, [])
    if options.hosts is not None:
        options.hosts = set(options.hosts)
        options.instances = sum(map(lambda x: CURDB.groups.get_host_instances(x), options.hosts), [])
    #        options.instances = filter(lambda x: x.host in options.hosts, sum(map(lambda x: x.get_used_instances() if options.run_int is True else x.get_used_base_instances(), options.intlookups), []))
    else:
        options.instances = None
    if options.functions is not None:
        options.functions = map(lambda x: functions.__dict__[x], options.functions.split(','))
    if options.xparams is None:
        options.xparams = {}
    else:
        options.xparams = eval(options.xparams)

    return options


def normalize(options):
    conn = sqlite3.connect(options.out_file)
    c = conn.cursor()

    cnt = c.execute("SELECT count(*) FROM sqlite_master WHERE type = 'table' AND name = ?", (options.table_name,)).fetchone()[0]
    if cnt > 0:
        nitems = c.execute("SELECT count(*) FROM %s WHERE sid = ?" % options.table_name, (options.sid,)).fetchone()[0]
        if nitems > 0:
            raise Exception("Table <%s> in file <%s> contains %s instances (should contain 0)" % (options.table_name, options.out_file, nitems))


def main(options):
    analyzer = Analyzer(options.region_file)
    analyzer.load_from_runner(options.intlookups, run_instances=options.instances, run_functions=options.functions,
                              quiet=options.quiet, timeout=options.timeout, run_int=options.run_int,
                              xparams=options.xparams)
    analyzer.save_to_sqlite(options.out_file, options.table_name)


if __name__ == '__main__':
    from core.db import CURDB

    options = parse_cmd()

    normalize(options)

    main(options)
