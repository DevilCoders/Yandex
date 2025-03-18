import os
import sys
import logging
import json
from argparse import ArgumentParser
from stubmaker.builder import override_module_import_path, traverse_modules

from ..builder.tk_representations_tree_builder import TolokaKitRepresentationTreeBuilder
from ..viewers.markdown_viewer import TolokaKitMarkdownViewer
from .. import constants
from ..util import GitHubSourceFinder


def make_markdowns(module_root, src_root, output_dir, skip_modules, modules_aliases_mapping, broken_modules, github_source_url):
    # Making sure our module is imported from provided src-root even
    # another version of the module is installed in the system
    override_module_import_path(module_root, src_root)

    for module_name, module in traverse_modules(module_root, src_root, skip_modules=skip_modules):

        # Normalizing paths before comparison and ensuring dst directory's existence
        dst_dir = os.path.abspath(output_dir)
        os.makedirs(dst_dir, exist_ok=True)

        builder = TolokaKitRepresentationTreeBuilder(
            module_name, module, module_root, modules_aliases_mapping=modules_aliases_mapping,
            preserve_forward_references=False,
        )
        viewer = TolokaKitMarkdownViewer(
            source_link_finder=GitHubSourceFinder(
                github_source_url, handle_unknown_source='ignore'
            ) if github_source_url else None
        )

        for name, markdown in viewer.get_markdown_files_for_module(builder.module_rep):

            if name in broken_modules:
                logging.warning(f'Module {name} is known to be generated with errors! Please review output manually')

            dst_path = f'{dst_dir}/{name}.md'
            logging.info(f'Creating: {dst_path}')
            with open(dst_path, 'w') as md_flo:
                md_flo.write(markdown)


def main():
    parser = ArgumentParser()
    parser.add_argument('--module-root', type=str, required=True, help='Module name to import these sources as')
    parser.add_argument('--src-root', type=os.path.abspath, required=True, help='Path to source files to process')
    parser.add_argument('--output-dir', type=os.path.abspath, required=True)
    parser.add_argument('--skip-modules', type=str, nargs='*', help='Module names to skip when generating markdowns', default=constants.markdowns.skip_modules)
    parser.add_argument('--modules-aliases', type=os.path.abspath, required=False, help='Path to module names to aliases mapping in json format.', )
    parser.add_argument('--logging-level', type=lambda x: str(x).upper(), required=False, choices=['DEBUG', 'INFO', 'WARNING', 'ERROR'], default='WARN')
    parser.add_argument('--github-source-url', type=str, required=False, help='Url of GitHub source root. If provided links to source code will be genreated.')

    args = parser.parse_args()

    logging.basicConfig(format='%(message)s', level=getattr(logging, args.logging_level), stream=sys.stderr)

    make_markdowns(
        module_root=args.module_root,
        src_root=args.src_root,
        output_dir=args.output_dir,
        skip_modules=args.skip_modules,
        modules_aliases_mapping=args.modules_aliases and json.load(open(args.modules_aliases)),
        broken_modules=constants.markdowns.broken_modules,
        github_source_url=args.github_source_url,
    )
