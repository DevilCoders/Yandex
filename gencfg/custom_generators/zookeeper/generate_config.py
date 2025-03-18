#!/skynet/python/bin/python

import os
import sys
from optparse import OptionParser

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

import gencfg
from gaux.aux_utils import run_command


def parse_cmd():
    usage = 'Usage: %prog [options]'
    parser = OptionParser(usage=usage)

    parser.add_option('-p', '--project', type='str', dest='project', default=None,
                      help='Obligatory. Project name')
    parser.add_option('-o', '--project-option', type='str', dest='project_option', default=None,
                      help='Obligatory. Project option (subproject name or something')
    parser.add_option('-d', '--dest-dir', type='str', dest='dest_dir', default=None,
                      help='Obligatory. Destination dir')

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    (options, args) = parser.parse_args()

    if options.project is None:
        raise Exception('Option --project is obligatory')
    if options.dest_dir is None:
        raise Exception('Option --dest-dir is obligatory')
    if len(args):
        raise Exception('Unparsed args: %s' % args)

    return options


if __name__ == '__main__':
    options = parse_cmd()

    print "Generating zookeeper config for project %s%s" % (options.project,
                                                            '' if options.project_option is None else ' (subproject %s)' % options.project_option)

    project_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'projects', options.project)
    command = [os.path.join(project_dir, '%s.py' % options.project)]

    command.append('-o')
    command.append(options.project_option)

    command.append('-d')
    command.append(options.dest_dir)

    try:
        retcode, out, err = run_command(command)
        sys.exit(retcode)
    except Exception, e:
        print e
        sys.exit(1)
