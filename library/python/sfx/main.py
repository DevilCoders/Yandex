import argparse

from .extract_program import extract_program_sources


def main():
    parser = argparse.ArgumentParser()
    arg = parser.add_argument
    arg('program', help='binary python program path')
    arg('-o', '--output', default='.', help='output directory')
    arg('-s', '--symlink', help='source root to symlink to')
    arg('--keep_src_path', action='store_true')
    options = parser.parse_args()

    extract_program_sources(
        options.program,
        options.output,
        sym_source_root=options.symlink,
        keep_src_path=options.keep_src_path,
    )
