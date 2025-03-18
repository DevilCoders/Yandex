import nirvana.job_context as nv
import logging
import subprocess
import os

logging.getLogger().setLevel(logging.INFO)


if __name__ == '__main__':
    job_context = nv.context()
    inputs = job_context.get_inputs()
    outputs = job_context.get_outputs()
    params = job_context.get_parameters()

    cmd = ["spark-submit-yt",
           "--deploy-mode", params.get("spyt-deploy-mode"),
           "--proxy", params.get("spyt-proxy"),
           "--discovery-path", params.get("spyt-discovery-path"),
           ]

    spyt_version = params.get("spyt-version")
    if spyt_version:
        cmd += ["--conf", spyt_version]

    py_files = params.get("spyt-py-files")
    if py_files:
        cmd += ["--py-files"] + py_files

    spark_conf = params.get("spyt-spark-conf")
    if spark_conf:
        for conf in spark_conf:
            cmd += ["--conf", conf]

    klass = params.get("spyt-class")
    if klass:
        cmd += ["--class", klass]

    secret = params.get("spyt-secret")
    if secret:
        os.environ["SPARK_SECRET"] = secret

    cmd += [params.get("spyt-driver-file")]

    driver_args = params.get("spyt-driver-args")
    if driver_args:
        for arg in driver_args:
            cmd += arg.split()
    try:
        input_driver_args = inputs.get_list("driver_args")
    except KeyError:
        input_driver_args = None
    if input_driver_args:
        driver_args = []
        for arg_file in input_driver_args:
            with open(arg_file) as f:
                args = []
                for s in f.read().splitlines():
                    args += s.split()
                driver_args += args
        cmd += driver_args

    yt_token = params.get("spyt-yt-token")
    os.environ["YT_TOKEN"] = yt_token

    logging.info("executing command: {}".format(" ".join(cmd)))

    code = subprocess.call(cmd)

    exit(code)
