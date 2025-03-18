from os import path

import yatest.common as yac

INPUT_EXAMPLE_JSON = 'input.example.json'


def test_example_input(binary_path, impl, binary=None):
    """
    Execute tasklet with input.example.json and compare with canonical data
    :param binary_path: path to tasklet source root
    :param impl: tasklet implementation name
    :param binary: executable binary name
    :return:
    """
    if binary is None:
        binary = path.basename(binary_path)
    bin = yac.binary_path(path.join(binary_path, binary))
    return yac.canonical_execute(bin, save_locally=True, args=_run_args(binary_path, impl), env=_cli_run_env())


def _cli_run_env():
    return {'LOGS_DIR': yac.test_output_path()}


def _run_args(binary_path, impl):
    with open(yac.source_path(path.join(binary_path, INPUT_EXAMPLE_JSON))) as input_example:
        return ['run', '--test', impl, '--input', input_example.read()]
