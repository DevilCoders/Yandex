#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
Combaine config generator
"""

import argparse
import difflib
import logging as log
import os
import pprint
import sys
from datetime import datetime

import yaml

SUPPORTED_FORMATS = [".yaml", ".json"]
TARGETS = ["parsing", "aggregate"]
EXIT_CODE = 0
DEBUG_TO_LEVELS = [log.ERROR, log.WARN, log.INFO, log.DEBUG]

log.basicConfig(
    format="%(asctime)s %(levelname)5s: %(lineno)4s#%(funcName)-12s %(message)s",
    level=log.ERROR,
)


class CombaineConfigGenerator():
    def __init__(self, args):
        self.args = args

    def process_file(self, generated, name, doc, now_mtime):
        """
        Process one file from configs repo
        """
        for section in TARGETS:
            if section not in doc:
                continue

            new_text = yaml.dump(doc[section])  # , default_flow_style=False, indent=4)
            cfg = os.path.join(self.args.output_dir, section, name)
            # cfg is full path with section, and it should be unique
            # then it safe use set here
            generated["_known_configs"].add(cfg)
            changed = self.generate_config(cfg, new_text, now_mtime)

            if changed == "unchanged":
                continue

            if changed == "added":
                generated["_num_of_new"] += 1
            try:
                if not self.args.test:
                    with open(cfg, "w") as cnf:
                        cnf.write(new_text)
                generated[section].append(cfg)
            except:  # pylint: disable=bare-except
                log.exception("Failed to write generated config %s", cfg)

    @staticmethod
    def generate_config(path, new_text, now_mtime):
        """
        Generate configs from one file
        """
        if not os.path.exists(path):
            log.info("Add new config file %s", path)
            return "added"
        mod_mtime = datetime.fromtimestamp(os.path.getmtime(path))
        with open(path, "r") as cnf:
            old_text = cnf.read()

        diff = [
            x.strip() for x in difflib.unified_diff(old_text.splitlines(), new_text.splitlines(),
                                                    "Old", "New", str(mod_mtime), str(now_mtime))
        ]
        if diff:
            log.warn("File %s diff:\n\n%s\n", path, "\n".join(diff))
            return "updated"
        return "unchanged"

    def regenerate(self):
        """Run generatrion process"""
        generated = {
            "parsing": [],
            "aggregate": [],
            "_known_configs": set(),
            "_num_of_new": 0,
        }
        for dirname, _, filenames in os.walk(os.path.join(self.args.source_dir)):
            if os.path.basename(dirname).startswith("."):
                log.debug("Skip directory '%s'", dirname)
                continue
            project = self.guess_name_of_project(dirname)
            for filename in filenames:
                conf = os.path.join(dirname, filename)
                self.regenerate_file(generated, project, conf)
        added_updated_configs = [
            "{}: {}".format(x[0], len(x[1])) for x in generated.items() if not x[0].startswith("_")
        ]
        log.warning("Added/Updated configs: %s (new %s), total: %s", added_updated_configs,
                    generated["_num_of_new"],
                    len(generated["parsing"]) + len(generated["aggregate"]))

        return generated["_known_configs"]

    def regenerate_file(self, generated, project, fname):
        """
        Regenerate one file

        """
        global EXIT_CODE
        name = os.path.basename(fname)
        _, ext = os.path.splitext(name)
        log.debug("Handle file: %s", fname)
        if not os.path.isfile(fname) or ext not in SUPPORTED_FORMATS:
            log.debug("Skip file %s", fname)
            return

        try:
            for idx, doc in enumerate(yaml.load_all(open(fname, "r"))):
                if not doc:
                    log.warning("Skip empty yaml doc from %s", fname)
                    continue
                doc_name = doc.get("name", None)
                if idx > 0 and not doc_name:
                    log.error("Missing name in %s, skip:\n%s", fname, yaml.dump(doc))
                    return

                conf_name = "{0}{1}".format(doc_name, ext) if doc_name else name
                now_mtime = datetime.fromtimestamp(os.path.getmtime(fname))

                self.ensure_juggler_project_tag(project, conf_name, doc)
                self.process_file(generated, conf_name, doc, now_mtime)
        except yaml.scanner.ScannerError:
            log.exception("Invalid yaml format: %s", fname)
            if not EXIT_CODE:
                EXIT_CODE = 1
        except Exception as exc:  # pylint: disable=broad-except
            if self.args.debug > 1:
                log.exception("Failed to update doc %s", exc)
            else:
                log.error("Failed to update doc %s", exc)
            EXIT_CODE = 2

    def guess_name_of_project(self, directory):
        """
        Try guess name of project for current configs
        """
        prefix = os.path.abspath(self.args.source_dir) + "/"
        directory = os.path.abspath(directory)
        suffix = directory.replace(prefix, "")
        project = suffix.split("/")[0]
        log.debug("Guess project '%s' from directory name '%s'", project, directory)
        return project

    @staticmethod
    def ensure_juggler_project_tag(project, path, config):
        """COMBAINE-129 Add projects tag if need"""
        senders = config.get("aggregate", {}).get("senders")
        if not senders:
            if config.get("aggregate"):
                return "Aggregate config has no 'senders' section"
            return

        for name, sender in senders.items():
            type_ = sender.get("type")
            if not type_:
                log.error("Unknown sender type in %s: %s", name, path)
                continue
            if type_ != "juggler":
                continue

            tags = sender.get("tags", [])
            if not tags or project not in tags:
                if not project:
                    log.error("Failed to guess project for %s", path)
                    continue
                if not tags:
                    sender["tags"] = []
                log.debug("Add tag %s for %s", project, path)
                sender["tags"].append(project)

    def cleanup(self, actual_configs):
        """Clean old configs"""
        alone_configs = []
        present_configs = set()

        for section in TARGETS:
            for dirname, _, filenames in os.walk(os.path.join(self.args.output_dir, section)):
                for filename in filenames:
                    present_configs.add(os.path.join(dirname, filename))

        alone_configs = list(present_configs - actual_configs)
        log.debug("Have\nknown_configs: %s\npresent_configs: %s\nalone_configs: %s\n",
                  pprint.pformat(actual_configs), pprint.pformat(present_configs),
                  pprint.pformat(alone_configs))

        for cnf in alone_configs:
            try:
                if not self.args.test:
                    os.remove(cnf)
                log.warning("Removed stale config: %s", cnf)
            except:  # pylint: disable=bare-except
                log.exception("Failed to remove stale config %s", cnf)

        if alone_configs:
            log.warning("Clean %s combaine configs", len(alone_configs))

    def run(self):
        for section in TARGETS:
            directory = os.path.join(self.args.output_dir, section)
            if not os.path.exists(directory):
                log.info("Make directory %s", directory)
                if not self.args.test:
                    os.makedirs(directory, mode=0o775)
        generated = self.regenerate()
        self.cleanup(generated)

        log.info("Complete config generation")


def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("-t", "--test", action='store_true', help="Only validate configs (dry run)")
    parser.add_argument("-d", "--debug", action='count', help="Print debug messages", default=0)
    parser.add_argument("-s",
                        "--source_dir",
                        default="/var/cache/media-combainer",
                        help="Configs writen by people")
    parser.add_argument("-o",
                        "--output_dir",
                        default="/etc/combaine",
                        help="Place for generated configs")

    args = parser.parse_args(sys.argv[1:])
    log.getLogger().setLevel(DEBUG_TO_LEVELS[args.debug or 0])
    log.debug("Given arguments %s", args)

    CombaineConfigGenerator(args).run()
    if args.test:
        sys.exit(EXIT_CODE)


if __name__ == '__main__':
    main()
