#!/usr/bin/env python

import re
import os
import sys
import socket
import subprocess
import time
from collections import defaultdict, OrderedDict
import functools
import random
import shutil

from argparse import ArgumentParser, RawTextHelpFormatter

import aux_arch
import aux_utils
from configuration import get_configuration

BASE_PORT = 13333
COEFF = 0.4119  # coeff for converting rps with current base to etalon gencfg rps


class EActions(object):
    SHOW_CONF = 'show_conf'  # show hardware confituration
    RUN = 'run'  # run params specfied from arguments
    RUN_GENCFG = 'run_gencfg'  # multiple runs to test for gencfg
    ALL = [SHOW_CONF, RUN, RUN_GENCFG]


def is_basesearch_started(port):
    """Check whether basesearch is started or not"""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect(('127.0.0.1', port))
        return True
    except Exception:
        return False


def start_stress_ng(bin_dir, cores, verbose=0):
    """Start stress_ng on list of specified cores

    :param cores: list of core ids to start stress_ng on"""

    stress_ng_binary = os.path.join(bin_dir, 'bin', aux_arch.get_instruction_set(), 'utils', 'stress-ng')
    if not os.path.exists(stress_ng_binary):
        raise Exception('Linpack binary <{}> does not exists'.format(stress_ng_binary))

    stress_ng_args = ['numactl', '-C', ','.join(str(x) for x in cores), stress_ng_binary, '--matrix', str(len(cores)), '--matrix-method', 'hadamard', '--matrix-size', '4096']
    stress_ng_stdout = open('stress_ng.out', 'w')
    stress_ng_stderr = open('stress_ng.err', 'w')
    stress_ng = subprocess.Popen(stress_ng_args, stdout=stress_ng_stdout, stderr=stress_ng_stderr)

    if verbose > 0:
        print aux_utils.timed_text('Started stress_ng <{}> by command <{}>'.format(stress_ng.pid, stress_ng_args))

    return stress_ng


def stop_stress_ng(stress_ng):
    """Stop stress_ng"""
    stress_ng.poll()
    if (stress_ng.returncode is not None) and (stress_ng.returncode != 0):
        print aux_utils.red_text('ERROR: Linpack with pid <{}> exited prematurely with status <{}>'.format(stress_ng.pid, stress_ng.returncode))
    else:
        stress_ng.kill()
    print aux_utils.timed_text('Stopped stress_ng <{}>'.format(stess_ng.pid))


def start_basesearchers(bin_dir, index_dir, basesearchers_count, use_numa=False, custom_cores=None, basesearch_variant='', verbose=0):
    """Start basesearchers on consecutive ports

    :param bin_dir: directory with binaries
    :param index_dir: directory with database
    :param basesearchers_count: number of basesearchers to start
    :param use_numa: flag to bind to numa nodes
    :param custom_cores: run basesearchers on custom cores (mutually exclusive with use_numa=True)
    :param verbose: verbose level (0 is minimal)
    """

    if basesearchers_count > 1 and (custom_cores is not None):
        raise Exception('Only one basesearch can be started when specified <custom_cores> param')
    if use_numa and (custom_cores is not None):
        raise Exception('Params <use_numa> and <custom_cores> are mutually exclusive')

    basesearchers = []
    try:
        for i in range(basesearchers_count):
            port = BASE_PORT + i
            if is_basesearch_started(port):
                raise Exception('Something already started on port {}'.format(port))

            if use_numa:
                fixed_index_dir = '{}.{}'.format(index_dir, i)
                if not os.path.exists(fixed_index_dir):
                    print aux_utils.red_text('Directory <{}> does not exist, copying from <{}>. This can take a while...'.format(fixed_index_dir, index_dir))
                    shutil.copytree(index_dir, fixed_index_dir)
            else:
                fixed_index_dir = index_dir

            # remove old loadlogs
            try:
                os.remove('loadlog.{}'.format(i))
            except OSError:
                pass

            # create arguments list
            basesearch_command = os.path.join(bin_dir, 'bin', aux_arch.get_instruction_set(), basesearch_variant, 'basesearch')
            args = [
                    basesearch_command, '-d', os.path.join(bin_dir, 'apache.ywsearch.cfg'), '-V', 'IndexDir={}'.format(fixed_index_dir),
                    '-V', 'LoadLog=loadlog.{}'.format(i), '-V', 'PassageLog=passage.{}'.format(i), '-p', str(port),
                   ]

            if use_numa:
                # first prepare database by evicting memory
                with open(os.devnull, 'w') as fnull:
                    subprocess.check_call(['vmtouch', '-e', fixed_index_dir], stdout=fnull, stderr=subprocess.STDOUT)
                # update args
                numa_nodes_count = len(get_configuration().numa_topology.numa_nodes)
                numa_nodes_per_basesearch = numa_nodes_count / basesearchers_count
                numa_nodes_to_bind = ','.join(str(get_configuration().numa_topology.numa_nodes[x].id) for x in range(i * numa_nodes_per_basesearch, (i + 1) * numa_nodes_per_basesearch))
                args = ['numactl', '--membind={}'.format(numa_nodes_to_bind), '--cpunodebind={}'.format(numa_nodes_to_bind), '--'] + args
            elif custom_cores is not None:
                args = ['numactl', '-C', ','.join(str(x) for x in custom_cores)] + args

            # start basesearch
            basesearch_stdout = open('basesearch.{}.out'.format(i), 'w')
            basesearch_stderr = open('basesearch.{}.err'.format(i), 'w')
            basesearch = subprocess.Popen(args, stdout=basesearch_stdout, stderr=basesearch_stderr)
            for i in range(50):
                time.sleep(1)
                if is_basesearch_started(port):
                    break
                basesearch.poll()
                if basesearch.returncode is not None:
                    raise Exception('Basesearch (command <{}>) exited on start with code <{}>'.format(args, basesearch.returncode))
            else:
                basesearch.kill()
                raise Exception('Failed to start basesearch (command {}). See basesearch.out and basesearch.err for details'.format(args))
            basesearchers.append(basesearch)
            if verbose > 0:
                print aux_utils.timed_text('Started basesearch <{}> by command <{}>'.format(basesearch.pid, args))
    except:
        for basesearch in basesearchers:
            basesearch.kill()
        raise

    return basesearchers


def stop_basesearchers(basesearchers, verbose=0):
    """Stop basesearch"""
    for basesearch in basesearchers:
        basesearch.poll()
        if (basesearch.returncode is not None) and (basesearch.returncode != 0):
            print aux_utils.red_text('ERROR: Basesearch with pid <{}> exited prematurely with status <{}>'.format(basesearch.pid, basesearch.returncode))
        else:
            basesearch.kill()
        if verbose > 0:
            print aux_utils.timed_text('Finished basesearch <{}>'.format(basesearch.pid))


def convert_signal_to_yaml(signame, notb_data, tb_data, indent="            ", display_signame=None):
    if display_signame is None:
        display_signame = signame

    notbl = ", ".join(map(lambda x: "%.1f" % x[signame], notb_data))
    tbl = ", ".join(map(lambda x: "%.1f" % x[signame], tb_data))

    return """%s%s:
%s    tbon: [%s]
%s    tboff: [%s]""" % (indent, display_signame, indent, tbl, indent, notbl)


def parse_dumper_result(bin_dir, fname):
    try:
        ddumper_command = os.path.join(bin_dir, 'bin', aux_arch.get_instruction_set(), 'd-dumper')
        args = [ddumper_command, '-a', '-f', fname]
        stdoutdata, _ = aux_utils.run_command(args)
        # qps = re.search('requests/sec\s+(.*)', stdoutdata).group(1)
        avg_req_time = re.search('avg req. time\s+(.*)', stdoutdata).group(1)
        reqs30ms = re.search(' h\[7\]\s+\(30000\s+..40000\).*\((.*)\%\)', stdoutdata).group(1)
        reqs100ms = re.search(' h\[10\]\s+\(100000\s+..200000\).*\((.*)\%\)', stdoutdata).group(1)
        reqs300ms = re.search(' h\[12\]\s+\(300000\s+..650000\).*\((.*)\%\)', stdoutdata).group(1)
        return dict(reqs30ms=float(reqs30ms), reqs100ms=float(reqs100ms), reqs300ms=float(reqs300ms), avg_req_time=float(avg_req_time))
    except Exception:
        print 'Failed to read dolbilo results (command {} failed)'.format(args)
        raise


def parse_dumper_result_multi(bin_dir, fnames):
    """Parse multiple dump files and sum the results"""
    AVG_FIELDS = ['reqs30ms', 'reqs100ms', 'reqs300ms', 'avg_req_time']

    d = defaultdict(float)
    for fname_result in (parse_dumper_result(bin_dir, x) for x in fnames):
        for k, v in fname_result.iteritems():
            d[k] += v

    for k in AVG_FIELDS:
        d[k] /= len(fnames)

    return d


def parse_loadlog_result_multi(fnames):
    """Parse multiple loadlogs and calculate avg qps"""
    qps = 0
    for fname in fnames:
        requests_by_second = defaultdict(int)
        for line in open(fname).readlines():
            requests_by_second[line.split('\t')[1]] += 1
        requests_counts = requests_by_second.values()
        requests_counts.sort()
        qps += requests_counts[int(len(requests_counts) * 0.8)]

    return float(qps)


def run_dolbilo_step(dolbilo_commands, signals, verbose=0):
    """Run dolbilo step, start list of dolbilo commands, get signals results during run"""
    if verbose > 0:
        print aux_utils.timed_text('Started dolbilo step')

    dolbilo_processes = []
    for dolbilo_command in dolbilo_commands:
        fnull = open(os.devnull, 'wb')
        dolbilo_process = subprocess.Popen(dolbilo_command, stdout=fnull, stderr=subprocess.STDOUT)
        dolbilo_processes.append(dolbilo_process)
        if verbose > 0:
            print aux_utils.timed_text('Started dolbilo <{}> by command <{}>'.format(dolbilo_process.pid, dolbilo_command))


    result = defaultdict(list)
    while True:
        for signal_name, signal_func in signals.iteritems():
            result[signal_name].append(signal_func())

        time.sleep(1)

        if not len([x for x in dolbilo_processes if x.poll() is None]):  # everything finished
            break

    for i, dolbilo_process in enumerate(dolbilo_processes):
        if dolbilo_process.returncode != 0:
            msg = 'Command <{}> exited with code <{}>'.format(dolbilo_commands[i], dolbilo_process.returncode)
            print aux_utils.red_text(msg)
            raise Exception(msg)

    for k in result:
        values = sorted(result[k])
        median_value = values[len(values)/2]
        result[k] = median_value

    if verbose > 0:
        print aux_utils.timed_text('Finished dolbilo_step')

    return result


def run_dolbilo(name=None, ftemplate=None, bin_dir=None, queries=None, runs=None, dolbilo_threads=None, dolbilo_processes=None,
                basesearch_processes=None, signals=None, verbose=0):
    """Run dolbilo on started basesearch, calculate qps/other signals

    :param name: name of dolobilo run
    :param ftemplate: template for output files
    :param bin_dir: base directory with dolbilo utils and basesearch
    :param queries: number of queries for basesearch (per working thread)
    :param runs: number of runs
    :param dolbilo_threads: number of working threads, working in parallel
    :param dolbilo_processes: number of dolbilo processes (dolbilo is single-thread, so we need multiple processes when dolbilo_threads is high)
    :param baseseach_processes: number of basesearch processes
    :signals: dict with (signal_name -> signal_func) of signals to be calculated during run
    """

    if verbose > 0:
        print aux_utils.timed_text('Starting dolbilo <{}>'.format(name))

    if dolbilo_processes is None:
        dolbilo_processes = dolbilo_threads / 10 + (dolbilo_threads % 10 != 0)
        dolbilo_processes = max(1, dolbilo_processes)

    executor_command = os.path.join(bin_dir, 'bin', aux_arch.get_instruction_set(), 'd-executor')

    signals_data = defaultdict(float)
    qps_data = []
    for i in range(runs):
        if verbose > 0:
            print aux_utils.timed_text('Starting dolbilo <{}> run <{}>'.format(name, i))
        dolbilo_commands = []
        dolbilo_out_files = []
        dolbilo_command_tpl = [executor_command, '-p', os.path.join(bin_dir, 'plan.bin'), '-m', 'finger', '-H', '127.0.0.1',]
        if dolbilo_threads > 0:
            for dolbilo_process_id in xrange(dolbilo_processes):
                port = BASE_PORT + dolbilo_process_id % basesearch_processes
                local_threads = dolbilo_threads * (dolbilo_process_id + 1) / dolbilo_processes - dolbilo_threads * dolbilo_process_id / dolbilo_processes
                local_queries = queries * local_threads
                local_out_file = '{}-worker{}-run{}.out'.format(ftemplate, dolbilo_process_id, i)

                if local_threads > 0:
                    dolbilo_out_files.append(local_out_file)
                    dolbilo_commands.append(dolbilo_command_tpl + ['--seed', str(random.randrange(0, 1000000000)),'-P', str(port), '-s', str(local_threads), '-o', local_out_file, '-Q', str(local_queries)])
        else:
            dolbilo_commands.append(['sleep', '100'])

        for k, v in run_dolbilo_step(dolbilo_commands, signals, verbose=verbose).iteritems():
            signals_data[k] += v

        # calculate result qps
        if dolbilo_threads > 0:
            signals_data['qps'] = parse_loadlog_result_multi(['loadlog.{}'.format(x) for x in range(basesearch_processes)])
            q = parse_dumper_result_multi(bin_dir, dolbilo_out_files)
            for k, v in q.iteritems():
                signals_data[k] += v
            step_qps = signals_data['qps']
        else:
            step_qps = 0
        qps_data.append(step_qps)

        # remove temporary stuff
        for fname in dolbilo_out_files:
            try:
                os.remove(fname)
            except OSError:
                pass

    for signal_name in signals_data:
        signals_data[signal_name] /= runs

    signals_keys = sorted(set(signals_data.keys()) - set(['qps', 'avg_req_time']))
    signals_string = ", ".join(map(lambda x: "%s %.2f" % (x, signals_data[x]), signals_keys))

    print '{name} ({dolbilo_threads}): all qps <{qps_list}>, (max qps {max_qps}, fixed qps {fixed_qps:.2f}, avg. req. time {avg_req_time:.2f} ms, {signals_string})'.format(
        name=name, dolbilo_threads=dolbilo_threads, qps_list=' '.join('{:.2f}'.format(x) for x in qps_data), max_qps=max(qps_data),
        fixed_qps=max(qps_data) * COEFF, avg_req_time=signals_data['avg_req_time'], signals_string=signals_string,
    )

    signals_data['qps'] = max(qps_data) * COEFF

    return signals_data


def parse_cmd():
    parser = ArgumentParser(description="""Utility to test basesearch performance. Sample commands:
    - ./base.perftest.py -a show_conf  # show current configuration and detect which signals can be processed
    - ./base.perftest.py -a run_gencfg -q 1000 -r 3  # run for gencfg (check performance with 1,2,...,NCPU cores used)
    - ./base.perftest.py -q 10000 -r 3 --force-tb-on -c 1,3,5,7  # run on 1,3,5,7 cores with tb on
    - ./base.perftest.py -q 10000 -r 3 --basesearcher-processes 2 --optimize-numa  # run with two basesearchers binded to corresponding numa nodes""",
                            formatter_class=RawTextHelpFormatter)

    parser.add_argument('-a', '--action', type=str, choices=EActions.ALL, required=True,
                        help='Obligatory. Action to execute')

    default_index_dir = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'database')
    parser.add_argument('-i', '--index-dir', type=str, default=default_index_dir,
                        help='Optional. Directory with database (<{}> by default)'.format(default_index_dir))

    default_bin_dir = os.path.dirname(os.path.realpath(__file__))
    parser.add_argument('-b', '--bin-dir', type=str, default=default_bin_dir,
                        help='Optional. Directory with binaries (<{}> by default)'.format(default_bin_dir))

    parser.add_argument('-n', '--name', type=str, default='test',
                        help='Optional. Name of run (<test> by default)')
    parser.add_argument('-q', '--queries', type=int, default=5000,
                        help='Optional. Number of queries (used int <{},{}> actions)'.format(EActions.RUN, EActions.RUN_GENCFG))
    parser.add_argument('-r', '--runs', type=int, default=3,
                        help='Optional. Number of runs (used int <{},{}> actions)'.format(EActions.RUN, EActions.RUN_GENCFG))
    parser.add_argument('-e', '--heatup', action='store_true', dest='heatup', default=False,
                        help='Optional. Heatup before run tests(for <{},{}> actions)'.format(EActions.RUN, EActions.RUN_GENCFG))
    parser.add_argument('-c', '--run-custom-cores', type=str, default=None,
                        help='Optional. Run on specified number of cores (comma-separated list)')
    parser.add_argument('-u', '--utility-cores-map', type=str, default=None,
                        help=('Optional. Distribution of utilities among cores (mutually exclusive with run_custom_cores). E. g. <b=1,2,3;l=0,5,6>'
                              'to run basesearch on cores <1,2,3> and stress_ng on <0,5,6>'))
    parser.add_argument('--dolbilo-processes', type=int, default=None,
                        help='Optional. Specify number of dolbilo processes (otherwise it will be caltulated automatically)')
    parser.add_argument('--basesearch-processes', type=int, default=1,
                        help='Optional. Specify number of baseserchers to start (one by default)')
    parser.add_argument('--force-tb-on', action='store_true', default=False,
                        help='Optional. Force set TB on before start')
    parser.add_argument('--force-tb-off', action='store_true', default=False,
                        help='Optional. Fet TB off before start')
    parser.add_argument('--optimize-numa', action='store_true', default=False,
                        help='Optional. Bind basesearchers (if started more than one) to numa nodes to improve memory access')
    parser.add_argument('--extra-signals', type=str, default=None,
                        help='Optional. List of extra signals to calculate (separated by <::>). See help for more information')
    parser.add_argument('--basesearch-variant', type=str, default='',
                        help='Optional. Use basesearch from <bin/arch/basesearch_variant/basesearch> rather than from <bin/arch/basesearch>')
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Increase output verbosity (maximum 1)")

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    options = parser.parse_args()

    return options


def normalize(options):
    options.basesearch_bind_cores = None
    options.stress_ng_bind_cores = None

    if options.utility_cores_map is not None:
        if options.run_custom_cores is not None:
            raise Exception('Options <--utility-cores-map> and <--run-custom-cores> are mutually exclusive')
        if options.action != EActions.RUN:
            raise Exception('Option <--utility-cores-map> is incompatable with <--action {}>'.format(options.action))

        basesearch_cores = None
        stress_ng_cores = None
        for elem in options.utility_cores_map.split(';'):
            binary_type, _, cores_list = elem.partition('=')
            cores_list = list(sorted({int(x) for x in cores_list.split(',')}))

            if binary_type == 'b':
                basesearch_cores = cores_list
            elif binary_type == 'l':
                stress_ng_cores = cores_list
            else:
                raise Exception('Failed to parse <--utility-cores-map> argument: <{}>'.format(elem))

        if basesearch_cores is None:
            raise Exception('Basesearch cores not specified in <--utility-cores-map>')
        if stress_ng_cores is not None:
            intersect_cores = set(basesearch_cores) & set(stress_ng_cores)
            if intersect_cores:
                raise Exception('Found intersect cores: <{}>'.format(','.join(str(x) for x in sorted(intersec_cores))))

        options.run_custom_cores = [len(basesearch_cores)]
        options.basesearch_bind_cores = basesearch_cores
        options.stress_ng_bind_cores = stress_ng_cores
    else:
        if options.run_custom_cores is None:
            options.run_custom_cores = [aux_arch.get_cpu_count()]
        else:
            new_run_custom_cores = []
            for s in options.run_custom_cores.split(','):
                if s.find('.') >= 0:  # we can specify number of cores as <0.5> which means run on 50% of cores
                    new_run_custom_cores.append(int(round(float(s) * aux_arch.get_cpu_count())))
                else:
                    new_run_custom_cores.append(int(s))
            options.run_custom_cores = new_run_custom_cores

    extra_signals_str = options.extra_signals
    options.extra_signals = OrderedDict()
    if extra_signals_str is not None:
        for extra_signal_str in extra_signals_str.split('::'):
            if extra_signal_str.find('=') == -1:
                raise Exception('Could not parse <{}> as signal description'.format(extra_signal_str))
            signal_name, _, signal_code = extra_signal_str.partition('=')
            # get signal func code
            d = {}

            try:
                exec signal_code in d
            except SyntaxError, e:
                print aux_utils.red_text('ERROR: Invalid python code <{}>'.format(signal_code))
                raise

            signal_func = d['signal']
            options.extra_signals[signal_name] = signal_func

    # check mutuall exclusive optoins
    if options.force_tb_on and options.force_tb_off:
        raise Exception("Options --force-tb-on and --force-tb-off are mutually exclusive")

    # check for too many cores
    too_many_cores = [x for x in options.run_custom_cores if x > aux_arch.get_cpu_count()]
    if len(too_many_cores):
        raise Exception('Specified run on <{}> cores, while having only <{}> ones on current machine'.format(','.join(str(x) for x in too_many_cores), aux_arch.get_cpu_count()))

    # check for numa options
    if options.optimize_numa:
        numa_nodes_count = len(get_configuration().numa_topology.numa_nodes)
        if numa_nodes_count % options.basesearch_processes != 0:
            raise Exception('Number of numa nodes <{}> is not divisible by number of basesearch processes <{}>')

    # check for existing directories
    if options.action in (EActions.RUN, EActions.RUN_GENCFG):
        def check_path(path, msg, is_directory=True):
            if not os.path.exists(path):
                raise Exception('{}: directory <{}> does not exists'.format(msg, path))
            if is_directory:
                if not os.path.isdir(path):
                    raise Exception('{}: path <{}> is not a directory'.format(msg, path))
            else:
                if not os.path.isfile(path):
                    raise Exception('{}: path <{}> is not a file'.format(msg, path))

        check_path(options.bin_dir, 'Binary directory (--bin-dir)')
        check_path(options.index_dir, 'Index dir for basesearch')

        basesearch_path = os.path.join(options.bin_dir, 'bin', aux_arch.get_instruction_set(), 'basesearch')
        check_path(basesearch_path, 'Basesearch executable', is_directory=False)

    # check for signal funcs to work
    for signal_name, signal_func in options.extra_signals.iteritems():
        try:
            value = signal_func()
        except Exception:
            print aux_utils.red_text('ERROR: failed to run func for signal <{}>'.format(signal_name))
            raise

def main(options):
    conf = get_configuration()

    signals = OrderedDict([
        ('package_consumption', conf.package_consumption_func),
        ('memory_consumption', conf.memory_consumption_func),
        ('effective_freq', conf.effective_freq_func),
    ])

    signals.update(options.extra_signals)

    if options.action == EActions.SHOW_CONF:
        # print current configuration
        print conf
        # test signals
        print 'Signals:'
        for k, v in signals.iteritems():
            print '    Signal <{}>: {}'.format(k, v())
    elif options.action == EActions.RUN:
        random.seed(0)
        basesearch_processes = start_basesearchers(options.bin_dir, options.index_dir, options.basesearch_processes, options.optimize_numa,
                                                   options.basesearch_bind_cores, basesearch_variant=options.basesearch_variant, verbose=options.verbose)
        if options.stress_ng_bind_cores is not None:
            stress_ng_process = start_stress_ng(options.bin_dir, options.stress_ng_bind_cores, verbose=options.verbose)
        try:
            if options.heatup:
                run_dolbilo(name='Heatup', ftemplate='heatup-{}'.format(options.name), bin_dir=options.bin_dir, queries=options.queries,
                            runs=3, dolbilo_threads=conf.ncpu, dolbilo_processes=options.dolbilo_processes,
                            basesearch_processes=options.basesearch_processes, signals={}, verbose=options.verbose)

            # set turbo boost
            if options.force_tb_on:
                conf.set_turbo_boost_func(True)
            elif options.force_tb_off:
                conf.set_turbo_boost_func(False)

            for core_count in options.run_custom_cores:
                run_dolbilo(name='{} Cores'.format(core_count), ftemplate='custom-{}-cores-{}'.format(options.name, core_count),
                            bin_dir=options.bin_dir, queries=options.queries, runs=options.runs, dolbilo_threads=core_count,
                            dolbilo_processes=options.dolbilo_processes, basesearch_processes=options.basesearch_processes, signals=signals,
                            verbose=options.verbose)
        finally:
            stop_basesearchers(basesearch_processes)
            if options.stress_ng_bind_cores is not None:
                stop_stress_ng(stress_ng_process)
    elif options.action == EActions.RUN_GENCFG:
        basesearch_processes = start_basesearchers(options.bin_dir, options.index_dir, options.basesearch_processes, options.optimize_numa,
                                                   basesearch_variant=basesearch_variant, verbose=options.verbose)
        try:
            if options.heatup:
                run_dolbilo(name='Heatup', ftemplate='heatup-{}'.format(options.name), bin_dir=options.bin_dir, queries=options.queries,
                            runs=3, dolbilo_threads=conf.ncpu, dolbilo_processes=options.dolbilo_processes,
                            basesearch_processes=options.basesearch_processes, signals=[], verbose=options.verbose)

            print "Run with disabled TB"
            conf.set_turbo_boost_func(False)
            disabled_tb_stats = map(
                lambda x: run_dolbilo(name='NOTB {} Cores'.format(x), ftemplate='notb-{}-cores-{}'.format(x, options.name), bin_dir=options.bin_dir,
                                      queries=options.queries, runs=options.runs, dolbilo_threads=x, dolbilo_processes=options.dolbilo_processes,
                                      basesearch_processes=options.basesearch_processes, signals=signals, verbose=options.verbose),
                range(0, conf.ncpu + 1))

            print "Run with enabled TB"
            conf.set_turbo_boost_func(True)
            enabled_tb_stats = map(
                lambda x: run_dolbilo(name='TB {} Cores'.format(x), ftemplate='tb-{}-cores-{}'.format(x, options.name), bin_dir=options.bin_dir,
                                      queries=options.queries, runs=options.runs, dolbilo_threads=x, dolbilo_processes=options.dolbilo_processes,
                                      basesearch_processes=options.basesearch_processes, signals=signals, verbose=options.verbose),
                range(0, conf.ncpu + 1))

            dt = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())

            # convert signals data to something printable
            signals_data = []
            for signal_name in sorted(disabled_tb_stats[0].keys()):
                signals_data.append(convert_signal_to_yaml(signal_name, disabled_tb_stats, enabled_tb_stats))
            signals_data = '\n'.join(signals_data)
            powerbyload_data = convert_signal_to_yaml("qps", disabled_tb_stats, enabled_tb_stats, indent="    ", display_signame="powerbyload")
            max_qps = max(map(lambda x: x['qps'], enabled_tb_stats))
            hw_configuration = '{}, {}, {}'.format(conf.baseboard, conf.memory, conf.disks)

            print ('-   model: {model_name}\n'
                   '    fullname: {model_name}\n'
                   '    power: {max_qps}\n'
                   '    ncpu: {cpu_count}\n'
                   '    freq: {all_cores_tb_freq}\n'
                   '    consumption:\n'
                   '        -   configuration: {hw_configuration}\n'
                   '            date: Date {current_date}\n'
                   '{signals_data}').format(model_name=conf.cpu_model, max_qps=max_qps, cpu_count=conf.ncpu,
                                            all_cores_tb_freq=conf.turbo_freq[-1], hw_configuration=hw_configuration,
                                            current_date=dt, signals_data=signals_data)
        finally:
            stop_basesearchers(basesearch_processes)
    else:
        raise Exception('Unsupported action {}'.format(options.action))


if __name__ == '__main__':
    parsed_options = parse_cmd()

    normalize(parsed_options)

    main(parsed_options)
