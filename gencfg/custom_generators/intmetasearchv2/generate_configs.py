#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))  # noqa

import copy
import re
from collections import defaultdict
import multiprocessing

# Do not remove this import.
import gencfg  # noqa

from core.argparse.parser import ArgumentParserExt
import core.argparse.types
from core.db import CURDB
from core.exceptions import UtilNormalizeException
from config import GENERATED_DIR
from gaux.aux_utils import correct_pfname

from custom_generators.intmetasearchv2.config_template import TConfigTemplateNode, TConfigNode
from custom_generators.intmetasearchv2.root import TRootNode
from custom_generators.intmetasearchv2.myconfig import GENERATOR_NAME

PARALLEL_PROCESSES = 8
DEFAULT_GENERATED_DIR = os.path.join(GENERATED_DIR, GENERATOR_NAME)


class EActions(object):
    GEN_CONFIGS = "genconfigs"
    SHOW_ANCHORS = "showanchors"
    GEN_ANCHORS = "genanchors"
    UPDATE_TEMPLATES = "updatetemplates"
    CHECK_INTERSECTION = "checkintersect"
    PRECALC_CACHES = "precalccaches"
    ALL = [GEN_CONFIGS, SHOW_ANCHORS, GEN_ANCHORS, UPDATE_TEMPLATES, CHECK_INTERSECTION, PRECALC_CACHES]


def get_parser():
    parser = ArgumentParserExt(
        description="Create configs for int/meta searchers",
        epilog=(
            "Some docs can be found there: "
            "https://wiki.yandex-team.ru/jandekspoisk/sepe/gencfg/usagepatterns/#popravit/napisatkonfig"
        )
    )
    parser.add_argument(
        "-a", "--action",
        type=str,
        default=EActions.GEN_CONFIGS,
        choices=EActions.ALL,
        required=True,
        help="Action to execute [required]",
    )
    parser.add_argument(
        "-t", "--template-files",
        type=core.argparse.types.comma_list,
        default=None,
        help=(
            "Comma-separated list of template file names. "
            "If not specified, all files for db/configs/intmetasearchv2 are selected"
        )
    )
    parser.add_argument(
        "-c", "--config-names",
        type=core.argparse.types.comma_list,
        default=None,
        help="Comma-separated list of config names [optional]",
    )
    parser.add_argument(
        "-o", "--output-dir",
        type=str,
        default=DEFAULT_GENERATED_DIR,
        help="Optional (for action {}). Write configs into specified dir instead of {}".format(
            EActions.GEN_CONFIGS,
            os.path.join(GENERATED_DIR, GENERATOR_NAME),
        ),
    )
    parser.add_argument(
        "--extra-output-dir",
        type=str,
        help="Optional. Write filtered configs into specified dir"
    )
    parser.add_argument(
        "--anchors",
        type=core.argparse.types.comma_list,
        default=None,
        help="Generate and print to stdout specified anchors [optional]"
    )
    parser.add_argument(
        "--workers",
        type=int,
        default=PARALLEL_PROCESSES,
        help="Optional (for action {}). Run in specified number of parallel processes".format(EActions.GEN_CONFIGS)
    )
    parser.add_argument(
        "-v", "--verbose-level",
        action="count",
        default=0,
        help="Verbose mode. Multiple -v options increase the verbosity. The maximum is 1."
    )

    return parser


def normalize(options):
    if options.template_files is not None:
        options.template_files = map(lambda x: os.path.realpath(os.path.abspath(x)), options.template_files)
    else:
        # find all tepmplate files by scanning db directory recursively
        options.template_files = []
        for root, directories, filenames in os.walk(os.path.join(CURDB.get_path(), 'configs', 'intmetasearchv2')):
            for filename in filenames:
                filename = os.path.join(root, filename)
                if os.path.isfile(filename) and filename.endswith('.yaml'):
                    options.template_files.append(filename)

    for template_file in options.template_files:
        if not os.path.exists(template_file):
            raise UtilNormalizeException(correct_pfname(__file__), ["template_files"],
                                         "File <%s> does not exists" % template_file)

    if options.action in [EActions.GEN_CONFIGS, EActions.UPDATE_TEMPLATES]:
        if options.output_dir == DEFAULT_GENERATED_DIR:
            if not os.path.exists(options.output_dir):
                try:
                    os.makedirs(options.output_dir)
                except Exception:
                    pass

        if not os.path.exists(options.output_dir):
            raise UtilNormalizeException(correct_pfname(__file__), ["output_dir"],
                                         "Output directory <%s> does not exists" % options.output_dir)

    if options.action == EActions.GEN_ANCHORS:
        def transform(s):
            if s.find(':') > 0:
                file_name, _, path = s.partition(':')
                file_name = os.path.join(os.getcwd(), file_name)
                return "%s:%s" % (file_name, path)
            else:
                return "%s:%s" % (options.template_files[0], s)

        options.anchors = map(transform, options.anchors)


def main(options):
    retval = 0

    root_node = TRootNode(options.template_files, verbose=options.verbose_level > 0)
    my_nodes = [child for child in root_node.children if child.name in options.template_files]

    nodes_by_anchor = dict()
    for file_node in root_node.children:
        for config_node in file_node.children:
            nodes_by_anchor[config_node.get_anchor_name()] = config_node

    if options.action == EActions.GEN_CONFIGS:
        if options.config_names is None:
            print "Rendering configs into <{}>".format(options.output_dir)
            if options.extra_output_dir:
                print "    and <{}>".format(options.extra_output_dir)
        else:
            print "Rendering configs <%s> ..." % ",".join(
                map(lambda x: os.path.join(options.output_dir, x), options.config_names)
            )

        configs_count = 0
        generated_configs = []
        for my_node in my_nodes:
            if options.workers > 1:
                queue = multiprocessing.Queue()
                processes = []
                for i in range(options.workers):
                    p = multiprocessing.Process(
                        target=my_node.render_to_files, args=(i, options.workers, queue, options)
                    )
                    p.start()
                    processes.append(p)

                for i in range(options.workers):
                    worker_result = queue.get()
                    configs_count += worker_result.config_nodes_count
                    generated_configs += worker_result.generated_configs

                for p in processes:
                    p.join()
                    if p.exitcode != 0:
                        raise Exception("One of workers exited with non-zero code <%s>" % (p.exitcode))
            else:
                result = my_node.render_to_files(0, 1, None, options)
                configs_count += result.config_nodes_count
                generated_configs += result.generated_configs

        if (options.config_names is not None) and configs_count != len(options.config_names):
            raise Exception("Found only <%d> configs when need <%s>" % (configs_count, len(options.config_names)))

        if options.config_names is None:
            print "Generated %d configs into %d files (templates %s)" % (
                configs_count, len(generated_configs), ",".join(options.template_files)
            )
        else:
            print "Generated %d configs into %d files" % (configs_count, len(generated_configs))

    elif options.action == EActions.SHOW_ANCHORS:
        cwd_path = os.path.abspath(os.getcwd())
        if my_nodes[0].name.startswith(cwd_path):
            prefix = cwd_path + "/"
        else:
            prefix = ""

        file_name = my_nodes[0].name[len(prefix):]
        print "File %s anchors:" % (file_name)
        for child_node in my_nodes[0].children:
            print "    %s" % child_node.get_anchor_name(prefix=prefix)
    elif options.action == EActions.GEN_ANCHORS:
        anchors_to_print = copy.copy(options.anchors)
        for file_node in root_node.children:
            for config_node in file_node.children:
                if config_node.get_anchor_name() in anchors_to_print:
                    print "======================== (%s, %s) START =============================" % (
                        config_node.name, config_node.anchor
                    )
                    strict = isinstance(config_node, TConfigNode)
                    print config_node.render(strict=strict)
                    print "======================== (%s, %s) END ===============================" % (
                        config_node.name, config_node.anchor
                    )
                    anchors_to_print.remove(config_node.get_anchor_name())

        if len(anchors_to_print) > 0:
            raise Exception("Could not find anchors <%s>" % (",".join(anchors_to_print)))
    elif options.action == EActions.UPDATE_TEMPLATES:
        class EState(object):
            OUTSIDE = 0,  # outside of auto-generated stuff
            INSIDE = 1,  # inside of auto-generated stuff

        for template_file in options.template_files:
            config_lines = map(lambda x: x.rstrip(), open(template_file).readlines())

            state = EState.OUTSIDE
            template_name = None

            result = []
            for line in config_lines:
                if state == EState.INSIDE:  # we are inside of auto-generated template block, just skip it
                    if line == "# @template %s finish" % (template_name):
                        state = EState.OUTSIDE
                else:
                    if re.match(r"^#\s+@template .*$", line) is not None:
                        m = re.match(r"^#\s+@template\s+([^\s]+) ?(start)?", line)
                        if not m:
                            raise Exception("Can not parse line <%s> in config <%s>" % (line, options.template_file))

                        template_name = m.group(1)
                        if m.group(2) is not None:
                            state = EState.INSIDE

                        result.append("# @template %s start" % template_name)

                        anchor_name = "%s:%s" % (template_file, template_name)
                        if anchor_name not in nodes_by_anchor:
                            raise Exception("Anchor <%s> in line <%s> of file <%s> not found" % (
                                anchor_name, line, options.template_file)
                            )
                        config_node = nodes_by_anchor[anchor_name]
                        rendered_help = config_node.render(strict=isinstance(config_node, TConfigNode)).split("\n")
                        rendered_help = map(lambda x: "# %s" % x, rendered_help)
                        result.extend(rendered_help)

                        result.append("# @template %s finish" % template_name)
                    else:
                        result.append(line)

            if state != EState.OUTSIDE:
                raise Exception("Parser in <OUTSIDE> state with template <%s> at end of file <%s>" % (
                    template_name, options.template_file
                ))

            with open(template_file, "w") as f:
                print "Write modified file <%s>" % template_file
                f.write("\n".join(result))
    elif options.action == EActions.CHECK_INTERSECTION:
        # find config nodes to generate
        config_template_nodes = []
        for my_node in my_nodes:
            config_template_nodes.extend(filter(lambda x: isinstance(x, TConfigTemplateNode), my_node.children))

        file_to_templates = defaultdict(list)
        for config_template_node in config_template_nodes:
            filenames = map(lambda (_, lst): lst, config_template_node._load_instances(
                config_template_node._internal_fields.get('_instances', None)))
            filenames = sum(filenames, [])
            for filename in filenames:
                file_to_templates[filename].append(config_template_node.path)

        for filename in sorted(file_to_templates.keys()):
            templates = file_to_templates[filename]
            if len(templates) > 1:
                retval = 0
                print "File <%s> generated by more then one template:" % filename
                print "\n".join(map(lambda x: "    %s" % x, templates))
    elif options.action == EActions.PRECALC_CACHES:
        # nothing to do: all caching is performed
        # in constructor of custom_generators.intmetasearchv2.node.TRootNode
        pass
    else:
        raise Exception("Unknown action <%s>" % (options.action))

    return retval


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)

    sys.exit(result)
