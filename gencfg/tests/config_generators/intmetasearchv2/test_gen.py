import os
import tempfile
import shutil

import gencfg
from custom_generators.intmetasearchv2 import generate_configs


def _check_config_generation(test_dirname):
    """
        This function generates config with template config from <test_dirname> and checks if it differs from etalon from <test_dirname>

        :param test_dirname(str): filename path with files <config.yaml> and <etalon.cfg>
        :return nothing: on error exception is raised
    """

    config_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'configs', test_dirname, 'config.yaml')
    etalon_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'configs', test_dirname, 'etalon.cfg')
    output_dir = tempfile.mkdtemp()

    jsoptions = {
        "action": "genconfigs",
        "template_files": [config_file],
        "output_dir": output_dir,
    }
    generate_configs.jsmain(jsoptions)

    output_file = os.path.join(output_dir, os.listdir(output_dir)[0])

    if open(output_file).read().strip() != open(etalon_file).read().strip():
        raise Exception("Test <%s> failed: generated config <%s> and etalon config <%s> differ" % (
                        test_dirname, output_file, etalon_file))

    shutil.rmtree(output_dir)


def test_include1(curdb):
    _check_config_generation("test_include1")


def test_include2(curdb):
    try:
        _check_config_generation("test_include2")
    except Exception, e:
        assert str(e).startswith("Found self-include at path ")


def test_include3(curdb):
    try:
        _check_config_generation("test_include3")
    except Exception, e:
        assert str(e).startswith("Found self-include at path ")


def test_include4(curdb):
    _check_config_generation("test_include4")


def test_include5(curdb):
    _check_config_generation("test_include5")


def test_conditional1(curdb):
    _check_config_generation("test_conditional1")


def test_conditional2(curdb):
    _check_config_generation("test_conditional2")


def test_extended1(curdb):
    _check_config_generation("test_extended1")


def test_extended2(curdb):
    _check_config_generation("test_extended2")
