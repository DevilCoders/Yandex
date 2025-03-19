import os

import yatest.common

JOIN_MARKUPS_PATH = yatest.common.binary_path("cloud/ai/speechkit/stt/bin/join/join")


def list_dir(dir_name, logger):
    dir_path = yatest.common.source_path(os.path.join(yatest.common.context.project_path, dir_name))
    logger.debug('dir_path={}'.format(dir_path))
    for _, _, f in os.walk(dir_path):
        return f


def run_join_e2e_test(input_name, logger):
    input_path = yatest.common.source_path(os.path.join(yatest.common.context.project_path, input_name))
    output_path = yatest.common.test_output_path('output.txt')

    logger.debug(JOIN_MARKUPS_PATH)
    logger.debug(input_path)

    cmd = (
        JOIN_MARKUPS_PATH,
        '--input',
        input_path,
        '--bit_offset',
        '3000',
        '--output',
        output_path,
        '--distance',
        'Wordwise',
    )

    yatest.common.execute(cmd)
    return [yatest.common.canonical_file(output_path, local=True)]
