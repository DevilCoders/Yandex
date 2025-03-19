from typing import Optional
from typing import Iterable
import json
import re
import argparse
import os
from pathlib import Path, PurePosixPath
import sys
import shutil

from cookiecutter.exceptions import OutputDirExistsException
from cookiecutter.main import cookiecutter


# Cookiecutter
TEMPLATE_DIR = Path(__file__).parent.absolute() / "template"
TEMPLATE_CONFIG_PATH = TEMPLATE_DIR / "cookiecutter.json"


# nirvana relative folders
VH_DIR_FROM_NIRVANA = Path("vh")
WORKFLOWS_DIR_FROM_NIRVANA = VH_DIR_FROM_NIRVANA / "workflows"
VH_YA_MAKE_PATH_FROM_NIRVANA = VH_DIR_FROM_NIRVANA / "ya.make"


# from root
NIRVANA_DIR_FROM_ROOT = Path("cloud", "dwh", "nirvana")


SUPPORTED_LAYERS = [
    "cdm",
    "ods",
    "raw",
    "stg",
    "export",
]


def save_domain_ya_make(path: Path, modules: Iterable[str]):
    with path.open("w") as f:
        f.write("RECURSE(\n")
        for module in sorted(modules):
            f.write(f"    {module}\n")
        f.write(")\n")


def get_domain_ya_make_modules(path: Path) -> set[str]:
    if path.exists():
        data = path.read_text()
        modules = {
            module
            for module in re.findall(r"\n +(.*)", data)
            if not module.isspace()
        }
    else:
        modules = set()

    return modules


def add_module_to_domain_ya_make(domain_dir: Path, module: str):
    domain_ya_make_path = domain_dir / "ya.make"

    modules = get_domain_ya_make_modules(domain_ya_make_path)
    modules.add(module)

    save_domain_ya_make(domain_ya_make_path, modules)


def remove_module_from_domain_ya_make(domain_dir: Path, module: str):
    domain_ya_make_path = domain_dir / "ya.make"

    modules = get_domain_ya_make_modules(domain_ya_make_path)
    try:
        modules.remove(module)
    except KeyError:
        print(f"No module {module} in {domain_dir} file.")
        return

    save_domain_ya_make(domain_ya_make_path, modules)


class VHYaMake:
    def __init__(self, raw_data: str):
        pattern = r"(?P<before>.*)" \
                  r"#   Workflows\n" \
                  r"#   CDM\n(?P<cdm>.*)\n" \
                  r"#   ODS\n(?P<ods>.*)\n" \
                  r"#   RAW\n(?P<raw>.*)\n" \
                  r"#   STG\n(?P<stg>.*)\n" \
                  r"#   EXPORT\n(?P<export>.*)\n" \
                  r"(?P<after>\)\n\nPY_SRCS.*)"
        pattern = re.compile(pattern, re.DOTALL)
        match = re.search(pattern, raw_data)

        layer_modules = {}
        for layer in SUPPORTED_LAYERS:
            modules = match.group(layer)
            modules = {
                module.lstrip()
                for module in modules.split('\n')
                if not module.isspace()
            }
            layer_modules[layer] = modules

        self.before = match.group('before')
        self.layer_modules = layer_modules
        self.after = match.group('after')

    def save(self, file_path: Path):
        with file_path.open("w") as f:
            f.write(self.before)
            f.write("#   Workflows\n")

            for layer, modules in self.layer_modules.items():
                f.write(f"#   {layer.upper()}\n")
                for module in sorted(modules):
                    f.write(f"    {module}\n")

            f.write(self.after)

    def add_module(self, layer: str, module: str):
        if layer not in SUPPORTED_LAYERS:
            raise KeyError(f"Unsupported layer: {layer}")

        self.layer_modules[layer].add(module)

    def remove_module(self, layer: str, module: str):
        if layer not in SUPPORTED_LAYERS:
            raise KeyError(f"Unsupported layer: {layer}")

        try:
            self.layer_modules[layer].remove(module)
        except KeyError:
            print(f"vh/ya.make doesn't have module named {module}")


def update_recurse(domains: list[str], module: str):
    recurse_structure = domains + [module]
    domain_dir = WORKFLOWS_DIR_FROM_NIRVANA

    for i in range(len(recurse_structure) - 1):
        domain = recurse_structure[i]
        next_domain = recurse_structure[i + 1]

        domain_dir = domain_dir / domain

        add_module_to_domain_ya_make(domain_dir, next_domain)


class WFTemplate:
    def __init__(self, workflow_path: list[str]):
        *domains, module_name = workflow_path

        domain_dir_from_workflow = Path(*domains)
        domain_dir = WORKFLOWS_DIR_FROM_NIRVANA / domain_dir_from_workflow

        self.layer = domains[0]
        if self.layer not in SUPPORTED_LAYERS:
            raise KeyError(f"Layer {self.layer} is not supported")

        self.module_name = module_name
        self.module_path_from_workflows = domain_dir_from_workflow / module_name
        self.module_path_from_nirvana = domain_dir / module_name

        self.domain_dir_from_nirvana = domain_dir
        self.domains = domains

        self.vh_ya_make = VHYaMake(VH_YA_MAKE_PATH_FROM_NIRVANA.read_text())

    def create(self, tags: Optional[list[str]]):
        self._bootstrap_module(tags)
        update_recurse(self.domains, self.module_name)

        module_path_from_root = NIRVANA_DIR_FROM_ROOT / self.module_path_from_nirvana

        self.vh_ya_make.add_module(self.layer, str(PurePosixPath(module_path_from_root)))
        self.vh_ya_make.save(VH_YA_MAKE_PATH_FROM_NIRVANA)

    def _bootstrap_module(self, tags: Optional[list[str]]):
        if tags:
            tags = '"' + '", "'.join(set(tags)) + '",'

        resources_path_from_workflows = self.module_path_from_workflows / "resources"
        resource_path_from_root = NIRVANA_DIR_FROM_ROOT / self.module_path_from_nirvana / "resources"

        config = {
            "module_name": self.module_name,
            "tags": tags,
            # Paths are posix in nirvana and ya.make files so force to posix in case we are on Windows.
            "resources_path_from_root": str(PurePosixPath(resource_path_from_root)),
            "resources_path_from_workflows": str(PurePosixPath(resources_path_from_workflows)),
        }

        # Cookiecutter requires cookiecutter.json file in template directory
        with TEMPLATE_CONFIG_PATH.open('w') as f:
            json.dump(config, f)

        try:
            cookiecutter(
                str(TEMPLATE_DIR),
                output_dir=str(self.domain_dir_from_nirvana),
                no_input=True
            )
        except OutputDirExistsException:
            answer = input("Output directory already exists. Overwrite it? y/n ")
            if answer != "y":
                sys.exit()

            cookiecutter(
                str(TEMPLATE_DIR),
                output_dir=str(self.domain_dir_from_nirvana),
                no_input=True,
                overwrite_if_exists=True
            )
        finally:
            os.remove(TEMPLATE_CONFIG_PATH)

    def delete(self):
        # Remove module folder
        if self.module_path_from_nirvana.exists():
            shutil.rmtree(str(self.module_path_from_nirvana))

        remove_module_from_domain_ya_make(self.domain_dir_from_nirvana, self.module_name)

        module_path_from_root = NIRVANA_DIR_FROM_ROOT / self.module_path_from_nirvana

        self.vh_ya_make.remove_module(self.layer, str(module_path_from_root))
        self.vh_ya_make.save(VH_YA_MAKE_PATH_FROM_NIRVANA)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--module", type=str)

    subparsers = parser.add_subparsers(dest="command")

    create_parser = subparsers.add_parser("create")
    create_parser.add_argument("--tags", nargs="*", type=str, default=[])
    create_parser.add_argument("--no-auto-tags", action="store_true")

    subparsers.add_parser("delete")

    return parser.parse_args()


def main():
    args = parse_args()

    workflow_path = args.module.split(".")
    wf_template = WFTemplate(workflow_path)

    if args.command == "create":
        tags = args.tags
        if not args.no_auto_tags:
            tags += workflow_path
        wf_template.create(tags)
    elif args.command == "delete":
        wf_template.delete()


if __name__ == "__main__":
    main()
