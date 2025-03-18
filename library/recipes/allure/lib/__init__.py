import os
import time
import json
import tarfile
import contextlib
import subprocess
import logging

import yatest.common

import library.python.fs as fs
import library.python.testing.recipe as recipe


logger = logging.getLogger(__name__)


class AllureRecipeException(Exception):

    def __init__(self, msg):
        super(AllureRecipeException, self).__init__("[[bad]]{}[[rst]]".format(msg))


def _get_allure_command_line(command_line_tar):
    tar_dir = os.path.dirname(command_line_tar)
    with contextlib.closing(tarfile.open(command_line_tar)) as tf:
        tf.extractall(tar_dir)
    return os.path.join(tar_dir, "bin", "allure")


def _get_test_allure_dir():
    return yatest.common.work_path("allure")


def start(argv):
    recipe.set_env("ALLURE_REPORT_DIR", _get_test_allure_dir())


def stop(argv):
    # generate allure report in test output dir
    allure_command_line_tar = yatest.common.build_path("library/recipes/allure/allure_commandline/allure-commandline.tar.gz")
    if not os.path.exists(allure_command_line_tar):
        raise AllureRecipeException("Allure recipe misconfiguration, cannot find allure command line, use recipe.inc to avoid missing dependencies")
    allure_command_line = _get_allure_command_line(allure_command_line_tar)

    if yatest.common.context.get_context_key("modulo") > 1:
        allure_output_dir = yatest.common.output_path(
            "allure_report_{}".format(yatest.common.context.get_context_key("modulo_index"))
        )
    else:
        allure_output_dir = yatest.common.output_path("allure_report")

    fs.create_dirs(allure_output_dir)
    cmd = [
        allure_command_line, "generate", _get_test_allure_dir(), "-o", allure_output_dir, "--clean"
    ]

    env = os.environ.copy()
    env["JAVA_HOME"] = yatest.common.java_home()
    logger.debug("Executing %s with env %s", cmd, env)
    p = subprocess.Popen(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    logger.debug("Command stdout: %s", out)
    logger.debug("Command stderr: %s", err)
    if p.returncode != 0:
        if "Could not find any allure results" in err:
            raise AllureRecipeException("Could not find any allure results")
        else:
            raise AllureRecipeException("Could not generate allure report - exit code {}: {} {}".format(p.returncode, out, err))

    trace_file = yatest.common.context.get_context_key("ya_trace_path")
    allure_link = os.path.join(allure_output_dir, "index.html")

    trace = []
    with open(trace_file) as f:
        for line in f.readlines():
            trace.append(json.loads(line))
    for item in trace:
        if item["name"] == "subtest-finished" and "logs" in item["value"]:
            item["value"]["logs"]["allure"] = allure_link

    with open(trace_file, "w") as f:
        for item in trace:
            f.write(json.dumps(item) + "\n")

        # dump suite event with the link to the allure report
        f.write("\n{}\n".format(json.dumps({
            "name": "chunk-event",
            "timestamp": time.time(),
            "value": {
                "logs": {
                    "allure": allure_link
                }
            }
        })))
