#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))  # noqa

import re
from argparse import ArgumentParser
import errno
import jinja2

# Please do not remove this import, you'll break GenCFG
# It is a kimkim@ magic
import gencfg  # noqa

from core.db import CURDB
import core.argparse.types as argparse_types

TAGKEY = 'SET_CURRENT_TAG'
GROUPKEY = 'SET_GROUP_NAME'


class ConfigGenerator(object):
    def __init__(self, options):
        self.current_tag = CURDB.get_repo().get_current_tag()
        if self.current_tag is None:
            self.current_tag = 'trunk'
        self.groups = options.groups

        self.template = self._read_template(options.template)
        proto_template_name = options.template.replace(".cfg", ".proto")
        self.proto_template = None
        if os.path.exists(proto_template_name):
            self.proto_template = self._read_template(proto_template_name)

        self.instance_filter = options.instance_filter
        self.replica_neighbours = {}

        if options.instance_filter is not None:
            for group in self.groups:
                group.generate_searcherlookup()

        self.instances = []
        for group in self.groups:
            for intlookup in (CURDB.intlookups.get_intlookup(x) for x in group.card.intlookups):
                for shard_id in xrange(intlookup.get_shards_count()):
                    replica_instances = intlookup.get_base_instances_for_shard(shard_id)
                    for instance in replica_instances:
                        self.replica_neighbours[instance] = replica_instances

            group_instances = group.get_kinda_busy_instances()
            if self.instance_filter is not None:
                group_instances = self.instance_filter(group, group_instances)
            self.instances.extend(group_instances)

        if len(self.instances) == 0:
            raise Exception("Found empty instances list for group <{}>".format(
                ",".join(map(lambda x: x.name, self.groups))
            ))

        self.jinja_variables = {
            "current_tag": self.current_tag,
            "CURDB": CURDB,
            "intlookups": CURDB.intlookups,
        }

        if options.param is not None:
            param_name, _, param_value = options.param.partition('=')
            self.jinja_variables[param_name] = param_value

        self.custom_config = options.custom_config

    def _read_template(self, template_name):
        template = ""
        with open(template_name) as f:
            template = f.read()
        return re.sub(TAGKEY, self.current_tag, template)

    @staticmethod
    def _write_file(file_name, contents):
        with open(file_name, "w") as f:
            f.write(contents)

    def _render_config(self, output_dir, fname, template):
        config_body = jinja2.Template(template).render(**self.jinja_variables) + "\n"
        self._write_file(os.path.join(output_dir, fname), config_body)
        return config_body

    def write_configs(self, output_dir):
        from custom_generators.intmetasearchv2.aux_utils import may_be_guest_instance
        config_body = ""
        for instance in self.instances:
            # ================================= RX-595 START ===================================
            instance = may_be_guest_instance(instance)
            # ================================= RX-595 FINISH ==================================

            fname = self._config_name(instance)
            self.jinja_variables["instance"] = instance
            if instance in self.replica_neighbours:
                self.jinja_variables['instance_replicas'] = self.replica_neighbours[instance]
                self.jinja_variables['instance_replicas_str'] = ' '.join(
                    x.name() for x in sorted(self.replica_neighbours[instance])
                )

            config_body = self._render_config(output_dir, fname, self.template)
            if self.proto_template:
                fname_proto = self._config_name(instance, proto=True)
                self._render_config(output_dir, fname_proto, self.proto_template)

        if self.custom_config is not None:
            self._write_file(os.path.join(output_dir, self.custom_config), config_body)

    @staticmethod
    def _config_name(instance, proto=False):
        # TODO: use instance.short_name() ?
        return '{}:{}.{}'.format(
            instance.host.name.split('.')[0],
            instance.port,
            "cfg.proto" if proto else "cfg",
        )

    @staticmethod
    def _safe_remove(file_name):
        try:
            os.remove(file_name)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise

    def clean_configs(self, output_dir):
        for instance in self.instances:
            fname = self._config_name(instance)
            self._safe_remove(os.path.join(output_dir, fname))
            fname_proto = self._config_name(instance, proto=True)
            self._safe_remove(os.path.join(output_dir, fname_proto))


def parse_cmd():
    parser = ArgumentParser()

    parser.add_argument(
        "-a", "--action",
        type=str,
        required=True,
        choices=["build", "clean"],
        help="Action to execute (build configs or clean) [required]",
    )
    parser.add_argument(
        "-g", "--groups",
        type=argparse_types.groups,
        required=True,
        help="List of groups to process [required]",
    )
    parser.add_argument(
        "-t", "--template",
        type=str,
        required=True,
        help="File with config template [required]",
    )
    parser.add_argument(
        "-f", "--instance-filter",
        type=argparse_types.pythonlambda,
        default=None,
        help="Extra filter on group instances [optional]",
    )
    parser.add_argument(
        "-o", "--output-dir",
        type=str,
        required=True,
        help="Path to directory with generated configs [required]")
    parser.add_argument(
        "--custom-config",
        type=str,
        default=None,
        help="Generate configs with name <--custom-config> in addition to generating usual configs [optional]",
    )
    parser.add_argument(
        "-p", "--param",
        type=str,
        default=None,
        help="Parameter in format <name=value> to pass to jinja renderer",
    )

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    config_generator = ConfigGenerator(options)

    if options.action == "build":
        config_generator.write_configs(options.output_dir)
    elif options.action == "clean":
        config_generator.clean_configs(options.output_dir)
    else:
        raise Exception("Unknown action <%s>" % options.action)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
