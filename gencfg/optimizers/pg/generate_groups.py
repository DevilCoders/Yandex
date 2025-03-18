import os
from collections import defaultdict
from gaux.aux_colortext import red_text

from gaux.aux_utils import run_command
from config import MAIN_DIR

from core.db import CURDB

SOLVER_FUNCTIONS = {'noskip_score': ('noskip_score', {'wa': 1.}),
                    'noskip_score_multi_block': ('noskip_score_multi_block', {'wa': 1.}),
                    'noskip_score_web': ('noskip_score', {'wa': 1.5}),
                    'skip_cluster_score': ('skip_cluster_score', {'wmax': 5., 'ws': 5., 'mode': '"skip_cluster"'}),
                    'skip_cluster_and_subcluster_score': (
                    'skip_cluster_score', {'wmax': 3., 'ws': 3., 'mode': '"skip_cluster_and_subcluster"'}),
                    'skip_cluster_score_multi_block': (
                    'skip_cluster_score', {'wmax': 0.5, 'ws': 0.5, 'mode': '"skip_cluster_multi_block"'}),
                    'skip_cluster_score_web': (
                    'skip_cluster_score', {'wmax': 0.5, 'ws': 0.5, 'mode': '"skip_cluster"'}),
                    'skip_subcluster_score': (
                    'skip_cluster_score', {'wmax': 0.4, 'ws': 0.15, 'mode': '"skip_subcluster"'}),
                    'skip_host_score': ('skip_cluster_score', {'wmax': 0.5, 'ws': 0.5, 'mode': '"skip_multiblock"'}),
                    'skip_switch_score': ('skip_cluster_score', {'wmax': 0.5, 'ws': 0.5, 'mode': '"skip_multiblock"'}),
                    # FIXME: not impelemented
                    'single_replica': ('total_replicas', {'replicas': 1, 'wa': 200}),
                    'two_replicas': ('total_replicas', {'replicas': 2, 'wa': 200}),
                    'three_replicas': ('total_replicas', {'replicas': 3, 'wa': 200}),
                    'four_replicas': ('total_replicas', {'replicas': 4, 'wa': 200}),
                    'five_replicas': ('total_replicas', {'replicas': 5, 'wa': 200}),
                    'twenty_replicas': ('total_replicas', {'replicas': 20, 'wa': 200}),
                    'ams_tk_replicas': ('total_replicas', {'replicas': 2, 'wa': 200}),
                    'foreign_replicas': ('total_replicas', {'replicas': 4, 'wa': 200}),
                    'garbage_subclusters': ('allow_subclusters', {'allow': '"22,23,24,25,26,27,28,29"', 'wa': 100}),
                    'single_stoika': ('single_stoika', {'wa': 100}),
                    'no_slow_subclusters': ('allow_subclusters', {'disallow': '"22"', 'wa': 100}),
                    'skip_all_but_one_cluster': (
                    'skip_cluster_score', {'wmax': 0.5, 'ws': 0.5, 'mode': '"skip_all_but_one_cluster"'}),
                    'no_dublicate_hosts': ('no_dublicate_hosts', {'wa': 100}),
                    'skip_switchtype_score': (
                    'skip_cluster_score', {'wmax': 11.0, 'ws': 1.0, 'mode': '"skip_subcluster"'}),
                    }


def _write_func(name, ranges, params, indent):
    result = indent + name + ' = {\n'

    result += indent + '    ranges = {\n'
    for first, last in ranges:
        result += indent + '        { first = ' + str(first) + '; last = ' + str(last) + '; };\n'
    result += indent + '    };\n'

    for k, v in params.items():
        result += indent + '    ' + str(k) + ' = ' + str(v) + ';\n'
    result += indent + '};\n'

    return result


def _create_optimizer_config(blocks, params, shards_power_list, solvers):
    indent = ' ' * 4
    result = 'instance = {\n'

    # write groups information
    def resolve_type(name, t):
        if t == 'dc':
            initial = {}
            f = lambda x: x.dc
        elif t == 'queue':
            initial = {}
            f = lambda x: x.queue
        elif t == 'switch':
            initial = {}
            f = lambda x: x.switch
        elif t == 'switch_type':
            # we force switch_type to have fixed ids
            # it will be very strange to have swapped ids
            initial = {0: 1, 1: 2, -1: 3}
            f = lambda x: x.switch_type
        else:
            raise Exception('Undefined %s "%s".' % (name, t))
        return initial, f

    cluster_ids, cluster_f = resolve_type('cluster_type', params.get('cluster_type', 'dc'))
    subcluster_ids, subcluster_f = resolve_type('subcluster_type', params.get('subcluster_type', 'queue'))
    multiblock_ids = {}
    result += indent + 'blocks = {\n'
    for block in blocks:
        cluster_id = cluster_f(block)
        subcluster_id = subcluster_f(block)
        multiblock_id = block.get_multiblock_id()
        if cluster_id not in cluster_ids:
            cluster_ids[cluster_id] = len(cluster_ids) + 1  # id starts from 1
        if subcluster_id not in subcluster_ids:
            subcluster_ids[subcluster_id] = len(subcluster_ids) + 1  # id starts from 1
        if multiblock_id not in multiblock_ids:
            multiblock_ids[multiblock_id] = len(multiblock_ids) + 1  # id starts from 1
        result += '%s%s{ cluster_id = %s; subcluster_id = %s; weight = %s; single_stoika = %s; multi_block_id = %s; };\n' % \
                  (indent, indent, cluster_ids[cluster_id], subcluster_ids[subcluster_id],
                   block.power, block.single_stoika_group, multiblock_ids[multiblock_id])
    result += indent + '};\n'

    # write shards information
    result += indent + 'shards = {\n'
    for name, multi_blocks, shards in shards_power_list:
        for i in range(0, len(shards)):
            result += indent * 2 + '{ ' 'type = "' + name + '"; weight = ' + str(shards[i]) + '; multi_blocks = ' + str(
                multi_blocks) + '; };\n'
    result += indent + '};\n'

    # write calculation information

    shards_ranges_by_outfile = {}
    cur_count = 0
    for outfile, multi_blocks, shards in shards_power_list:
        shards_ranges_by_outfile[outfile] = (cur_count, cur_count + len(shards))
        cur_count += len(shards)

    reverted_solvers = defaultdict(list)
    for outfile, functions in solvers.items():
        for function in functions:
            if outfile in shards_ranges_by_outfile:
                reverted_solvers[function].append(outfile)

    result += indent + 'functions = {\n'

    for function, outfiles in reverted_solvers.items():
        ranges = map(lambda x: shards_ranges_by_outfile[x], outfiles)
        result += indent * 2 + '{\n'
        result += _write_func(SOLVER_FUNCTIONS[function][0], ranges, SOLVER_FUNCTIONS[function][1], indent * 3)
        result += indent * 2 + '};\n'

    result += indent + '};\n'

    # section for debug purposes; empty by default
    result += indent + 'params = {\n'

    timelimit_var = 'TIMELIMIT'
    if os.getenv(timelimit_var) is not None:
        timelimit = int(os.getenv(timelimit_var))
        assert (timelimit > 0)
        print red_text('Using %s variable = %s seconds' % (timelimit_var, timelimit))
        result += indent * 2 + 'timeLimit = %s;\n' % timelimit

    result += indent + '};\n'

    result += '};\n'

    return result


def run_solver(solver_name, input_file, output_file, solution_size):
    workers_var = 'WORKERS'

    if os.getenv(workers_var) is None:
        print red_text('%s variable is not set; running solver on localhost!' % workers_var)
        workers = ['localhost']
    else:
        def resolve_worker(w):
            w = w.strip()
            if not w:
                return []
            if CURDB.groups.has_group(w):
                return list(CURDB.groups.get_group(w).getHosts())
            if CURDB.hosts.has_host(w):
                return [CURDB.hosts.get_host_by_name(w)]
            raise Exception('Could not resolve "%s"' % w)

        workers = os.getenv(workers_var).strip().split(',')
        workers = set(sum([resolve_worker(x) for x in workers], []))
        if not workers:
            raise Exception('Got empty set of workers from "%s"' % os.getenv(workers_var))
        workers = [x for x in workers if x.os == os.uname()[0]]
        if not workers:
            raise Exception('No worker hosts from "%s" have same OS than localhost.' % workers.filename)
        workers = [x.name for x in workers]
        print 'Running solver on %d %s hosts: %s' % \
              (len(workers), os.uname()[0], ','.join(sorted(workers)))

    run_command(
        [os.path.join(MAIN_DIR, 'optimizers', 'pg', 'sky_optimizer.py'), '-i', input_file, '-o', output_file, '-w',
         ','.join(workers), '-z', str(solution_size), '-s', solver_name])


def generate_optimal_grouping(solver_name, params, blocks, shards_power_list, solvers):
    #    print _create_optimizer_config(blocks, shards_power_list)

    if os.getenv("FAKE_OPTIMIZATION") is not None:
        print red_text("!!!!! MAKING FAKE OPTIMIZATION !!!!")
        groupsets = []
        #    print shards_power_list
        total_groupsets = sum(map(lambda (x, y, z): len(z), shards_power_list), 0)
        for i in range(total_groupsets):
            groups = []
            for j in range(len(blocks) * i / total_groupsets, len(blocks) * (i + 1) / total_groupsets):
                groups.append(blocks[j])
            groupsets.append(groups)
        return groupsets

    #    groupsets = []
    #    total_groupsets = sum(map(lambda (x, y): len(y), shards_power_list))
    #    for i in range(0, total_groupsets):
    #        gygy = Dummy()
    #        gygy.groups = []
    #        groupsets.append(gygy)
    #    for i in range(0, len(blocks)):
    #        groupsets[i % total_groupsets].groups.append(blocks[i])
    #    return groupsets

    total_groupsets = sum(map(lambda (x, y, z): len(z), shards_power_list), 0)

    in_data = _create_optimizer_config(blocks, params, shards_power_list, solvers)

    IN_DATA_FILE = 'indata'
    OUT_DATA_FILE = 'outdata'
    f = open(IN_DATA_FILE, 'w')
    f.write(in_data)
    f.close()
    run_solver(solver_name, IN_DATA_FILE, OUT_DATA_FILE, total_groupsets)
    out_data = open(OUT_DATA_FILE).read()

    group_lines = out_data.rstrip('\n').split('\n')
    group_lines = group_lines[1:]  # without score
    print group_lines

    # get data
    result = []
    for line in group_lines:
        groups = []
        for group_id in line.rstrip().split(' '):
            groups.append(blocks[int(group_id)])
        result.append(groups)

    return result
