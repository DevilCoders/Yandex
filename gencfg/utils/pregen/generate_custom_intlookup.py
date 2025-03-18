#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'utils')))
import tempfile

from optparse import OptionParser

import gencfg
from core.db import CURDB
from config import TEMPFILE_PREFIX, MAIN_DIR

from optimizers.pg.intlookup_configurator import build_intlookup_from_config
from utils.common.show_stats import brief_statistics

OPTIMIZER_CONFIG_TEMPLATE = """<xml>

<params>
    <solver_name> %(optimizer)s </solver_name>
</params>

<intlookups>
    <intlookup>
        <file_name> %(output_file)s </file_name>

        <hosts_per_group> %(bases_per_group)s </hosts_per_group>
        <ints_per_group> %(ints_per_group)s </ints_per_group>
        <free_basesearchers_per_cluster> 0 </free_basesearchers_per_cluster>
        <brigade_groups_count> %(shards_count)s </brigade_groups_count>
        <base_type> %(base_type)s </base_type>
        <temporary> %(temporary)s </temporary>

        <weight> 1. </weight>

        <params>
            <create_multiqueue_groups> %(multi_queue)s </create_multiqueue_groups>
        </params>

        <generate>
            <method> optimizer </method>
            <optimizer>
                <solver_functions>
%(functions)s
                </solver_functions>
            </optimizer>
        </generate>
    </intlookup>

</intlookups>

</xml>
"""


class Options(object):
    def __init__(self):
        self.base_type = None
        self.brigade_size = None
        self.bases_per_group = 0
        self.ints_per_group = 0
        self.shards_count = 0
        self.optimizer_functions = None
        self.output_file = None
        self.multi_queue = False
        self.optimizer_executable = os.path.join(MAIN_DIR, 'optimizer')
        self.temporary = False
        self.verbose = False

    def parse_cmd(self):
        usage = "Usage %prog [options]"
        parser = OptionParser(usage=usage)

        parser.add_option("-t", "--base-type", type="str", dest="base_type", help="specify instances type")
        parser.add_option("-b", "--bases-per-group", type="int", dest="bases_per_group", help="specify hosts per group")
        parser.add_option("-i", "--int-per-group", type="int", dest="ints_per_group", help="specify ints per group")
        parser.add_option("-s", "--shard-count", type="str", dest="shards_count", help="specify total number of shards")
        parser.add_option("-f", "--optimizer-functions", type="str", dest="optimizer_functions",
                          help="specify optimizate functions")
        parser.add_option("-o", "--output-file", type="str", dest="output_file", help="specify output file")
        parser.add_option("-m", "--multi-queue", dest="multi_queue", action='store_true', default=False,
                          help="multi-queue groups")
        parser.add_option("-v", "--verbose", dest="verbose", action="store_true", default=False,
                          help="add verbose output")

        if len(sys.argv) == 1:
            sys.argv.append('-h')

        options, args = parser.parse_args()
        obligatory_params = ["base_type", "bases_per_group", "ints_per_group", "shards_count",
                             "optimizer_functions", "output_file"]
        for option in obligatory_params:
            if getattr(options, option) is None:
                raise Exception("Obligatory option '%s' is not set" % option)
        if len(args):
            raise Exception("Can not parse options: %s" % (' '.join(args)))

        self.base_type = options.base_type
        self.bases_per_group = options.bases_per_group
        self.ints_per_group = options.ints_per_group
        self.shards_count = options.shards_count
        self.optimizer_functions = options.optimizer_functions.split(',')
        self.output_file = options.output_file
        self.multi_queue = options.multi_queue
        self.verbose = options.verbose

    def check(self):
        if not os.path.exists(self.optimizer_executable):
            raise Exception('Optimizer executable file "%s" is not found' % self.optimizer_executable)
        if not self.optimizer_functions:
            raise Exception('Empty list of optimizer functions')


def generate_custom_intlookup(options):
    config_filename = None
    try:
        options.check()
        optimizer_functions = '\n'.join(
            map(lambda x: '%s<function> %s </function>' % (' ' * 20, x), options.optimizer_functions))
        config, config_filename = tempfile.mkstemp(prefix=TEMPFILE_PREFIX, suffix='.xml', dir=CURDB.CONFIG_DIR)
        os.write(config, OPTIMIZER_CONFIG_TEMPLATE % {
            'optimizer': options.optimizer_executable,
            'base_type': options.base_type,
            'bases_per_group': options.bases_per_group,
            'ints_per_group': options.ints_per_group,
            'shards_count': options.shards_count,
            'functions': optimizer_functions,
            'output_file': options.output_file,
            'multi_queue': int(options.multi_queue),
            'temporary': int(options.temporary)
        })
        os.close(config)
        build_intlookup_from_config(os.path.join(CURDB.CONFIG_DIR, config_filename))
        if not options.temporary:
            CURDB.intlookups.add_build_intlookup(CURDB.intlookups.get_intlookup(options.output_file))

        if options.verbose:
            print open(config_filename).read()

    finally:
        if config_filename is not None:
            os.unlink(config_filename)


if __name__ == '__main__':
    options = Options()
    options.parse_cmd()
    generate_custom_intlookup(options)
    CURDB.update()
    print 'Intlookup statistics:'
    brief_statistics(CURDB.intlookups.get_intlookup(options.output_file), False)
