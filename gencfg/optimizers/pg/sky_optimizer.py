#!/skynet/python/bin/python

import sys, os
from optparse import OptionParser
from gaux.aux_colortext import red_text
from kernel.util.errors import formatException
from config import TEMPFILE_PREFIX
import commands
import gaux.aux_utils

import api.kqueue


class Skyrun(object):
    def __init__(self, executable, data, solution_size, leave_files=False):
        sky_user_var = 'SKY_USER'
        if os.getenv(sky_user_var) is None:
            user = gaux.aux_utils.getlogin()
            print red_text('%s variable is not set; running solver under %s!' % (sky_user_var, user))
        else:
            user = os.getenv(sky_user_var)

        self.osUser = user
        self.executable = executable
        self.data = data
        self.solution_size = solution_size
        self.leave_files = leave_files

    def run(self):
        from tempfile import mkstemp

        exec_file, exec_filename = mkstemp('', prefix=TEMPFILE_PREFIX)
        os.write(exec_file, self.executable)
        os.close(exec_file)
        import stat
        os.chmod(exec_filename, os.stat(exec_filename).st_mode | stat.S_IXUSR)

        data_file, data_filename = mkstemp()
        os.write(data_file, self.data)
        os.close(data_file)

        command = "nice " + exec_filename + " " + data_filename
        (status, solution) = commands.getstatusoutput(command)

        if not self.leave_files:
            os.unlink(exec_filename)
            os.unlink(data_filename)

        if status != 0:
            raise Exception("Command <%s> finished with status %d" % (command, status))

        solution = solution.rstrip('\n').split('\n')[-self.solution_size:]
        return solution


def parse_cmd():
    parser = OptionParser(usage="""usage: %prog -z size -s solver -d data_file""")

    parser.add_option("-s", "--solver", dest="solver", default=None,
                      help="Obligatory. Solver")
    parser.add_option("-i", "--input-file", dest="input_file", default=None,
                      help="Obligatory. Input data file")
    parser.add_option("-o", "--output-file", dest="output_file", default=None,
                      help="Obligatory. Output data file")
    parser.add_option("-z", "--size", dest="size", default=None,
                      help="Obligatory. Problem size")
    parser.add_option("-w", "--workers", dest="workers", default=None,
                      help="Optional. Worker hosts")
    parser.add_option("-l", "--leave_files", dest="leave_files", default=False,
                      help="Optional. Leave garbage on remote hosts (for debug)")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    (options, args) = parser.parse_args()
    if options.size is None or options.solver is None or options.input_file is None or options.output_file is None:
        raise Exception('Some argument is missing')
    if options.workers is None:
        options.workers = ['localhost']
    else:
        options.workers = options.workers.split(',')
    return options


if __name__ == '__main__':
    options = parse_cmd()
    data = open(options.input_file).read()
    solver_name = options.solver
    solution_size = int(options.size) + 1
    workers = options.workers

    if not os.path.exists(solver_name):
        raise Exception("Solver executable <%s> does not exists" % solver_name)

    client = api.cqueue.Client('c')
    succeeded_workers = set()

    solution = None
    solution_score = None
    scores = []

    for worker, sub_solution, failure in client.run(workers, Skyrun(open(solver_name).read(), data, solution_size,
                                                                    leave_files=options.leave_files)).wait():
        if failure is not None:
            print 'HOST %s' % worker
            print formatException(failure)
        else:
            assert (sub_solution is not None)
            sub_solution_score = float(sub_solution[0].strip())
            scores.append(sub_solution_score)
            if not solution or sub_solution_score < solution_score:
                solution = sub_solution
                solution_score = sub_solution_score
            succeeded_workers.add(worker)
    if not succeeded_workers:
        raise Exception('No succeeded workers!')
    assert (solution is not None)
    print 'Hosts %d of %d succeded: %s' % (len(succeeded_workers), len(workers), ','.join(sorted(workers)))
    print 'Scores vary from %f to %f.' % (min(scores), max(scores))

    f = open(options.output_file, 'w')
    print >> f, '\n'.join(solution)
    f.close()
