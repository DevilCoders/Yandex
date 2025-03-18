#!/skynet/python/bin/python

import functools
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import multiprocessing

import gencfg
from core.db import CURDB
import core.argparse.parser
import core.argparse.types
from config import MAIN_DIR
from gaux.aux_colortext import red_text

import tools.cfggen.generators.basesearch
import tools.cfggen.generators.jinja_template


class EActions(object):
    BUILD = "build"  # build configs and write them to output directory
    CLEAN = "clean"  # clean built configs
    ALL = [BUILD, CLEAN]


GENERATORS = {
    'basesearch': tools.cfggen.generators.basesearch.TBasesearchGenerator(),
    'jinja_template': tools.cfggen.generators.jinja_template.JinjaTemplateGenerator(),
}


def get_parser():
    parser = core.argparse.parser.ArgumentParserExt(description="Generate config for specified group")

    parser.add_argument("-a", "--action", type=str, default=EActions.BUILD,
                        help="Obligatory. Action to execute (one of <%s>)" % ",".join(EActions.ALL))
    parser.add_argument("-g", "--groups", type=core.argparse.types.groups, required=True,
                        help="Obligatory. List of groups to generate configs for")
    parser.add_argument("-e", "--generator-type", type=str, default=None,
                        choices=sorted(GENERATORS.keys()),
                        help="Generate configs only with specified generator: one of <%s>"
                             % ",".join(sorted(GENERATORS.keys())))
    parser.add_argument("-o", "--output-dir", type=str, default=None,
                        help="Optional. Output directory (use default output directory if not specified")
    parser.add_argument("-w", "--workers", type=int, default=1,
                        help="Obligatory. Number of parallel workers. One worker by default")
    parser.add_argument("--strict", action="store_true", default=False,
                        help="Optional. Strict build (raise exception if a group without config section was found")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Verbosity level (maximum is 2)")
    parser.add_argument("-y", "--apply", action="store_true", default=False,
                        help="Optional. Write generated config to output dir")
    parser.add_argument("--filter-prj", action="append",
                        help="Filter groups by prj tag. Can be multiple.")

    return parser


def normalize(options):
    if options.output_dir is None:
        options.output_dir = os.path.join(MAIN_DIR, "w-generated", "all")

    # check if we can generate configs for specified groups
    if options.strict:
        bad_groups = []
        for group in options.groups:
            itype = group.parent.db.itypes.get_itype(group.card.tags.itype)
            if itype.config_type not in GENERATORS:
                bad_groups.append(group)
        if bad_groups:
            raise Exception("Do not know how to generate configs for groups <%s>" % ",".join(map(lambda x: x.card.name, bad_groups)))


def process_group(groupname, action, strict):
    try:
        group = CURDB.groups.get_group(groupname)
        itype = group.parent.db.itypes.get_itype(group.card.tags.itype)

        use_generator = itype.config_type
        if group.card.tags.itype == 'base' and 'jinja_template' in group.card.configs:
            # temporary hack for slow migrate from basesearch to jinja_template generator
            use_generator = 'jinja_template'

        if not use_generator and not strict:
            use_generator = 'basesearch'

        if itype.config_type in GENERATORS or not strict:
            if action == EActions.BUILD:
                return GENERATORS[use_generator].build(group)
            elif action == EActions.CLEAN:
                return GENERATORS[use_generator].gennames(group, add_custom_name=False)
            else:
                raise Exception("Unknown action <%s>" % action)
        else:
            raise Exception("Do not know how to generate config for group <%s>" % group.card.name)
    except Exception:
        print "Failed to build config for group <%s>" % groupname
        raise


def apply_filters(group, options):
    if options.generator_type is not None:
        if group.parent.db.itypes.get_itype(group.card.tags.itype).config_type != options.generator_type:
            return False

    if options.filter_prj:
        if not any(prj in options.filter_prj for prj in group.card.tags.prj):
            return False

    return True

def main(options):
    if not os.path.exists(options.output_dir):
        os.makedirs(options.output_dir)

    groups = [group for group in options.groups if group.card.configs.enabled and apply_filters(group, options)]
    group_names = [group.card.name for group in groups]

    if options.workers > 1:
        pool = multiprocessing.Pool(options.workers)
        map_routine = pool.map
    else:
        map_routine = map
    reports = map_routine(functools.partial(process_group, action=options.action, strict=options.strict), group_names)

    if options.apply:
        if options.action == EActions.BUILD:
            print "Generate configs for {} groups (into <{}>)".format(len(reports), options.output_dir)
            for report in reports:
                if options.verbose >= 1:
                    print "    Group {}: generated {} unique configs ({} total) for group".format(report.group_name,
                        len(report.report_entries), sum(map(lambda x: len(x.fnames), report.report_entries)))
                for report_entry in report.report_entries:
                    for fname in report_entry.fnames:
                        with open(os.path.join(options.output_dir, fname), 'wb') as f:
                            f.write(report_entry.content)
        elif options.action == EActions.CLEAN:
            print "Remove configs for {} groups (from <{}>)".format(len(reports), options.output_dir)
            to_remove = []
            for report in reports:
                to_remove.extend(report)

            for fname in to_remove:
                fname = os.path.join(options.output_dir, fname)
                try:
                    os.remove(fname)
                except OSError:
                    pass
        else:
            raise Exception("Unknown action <{}>".format(options.action))
    else:
        print red_text("Not updated! Add option -y to update.")

    return reports


def jsmain(json_options):
    options = get_parser().parse_json(json_options)
    normalize(options)
    return main(options)


def cli_main():
    options = get_parser().parse_cmd()
    normalize(options)
    main(options)


if __name__ == '__main__':
    cli_main()
