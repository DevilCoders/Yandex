import nirvana.job_context as nv
import logging
import subprocess

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

    cmd += [params.get("spyt-py-driver-file")]

    secret_arg = params.get("spyt-secret-py-driver-arg")
    secret_arg_ix = None
    if secret_arg:
        cmd += ["--secret", "***Secret Arg***"]
        secret_arg_ix = len(cmd) - 1

    driver_args = params.get("spyt-py-driver-args")
    if driver_args:
        for arg in driver_args:
            cmd += arg.split()
    try:
        input_driver_args = inputs.get_list("py_driver_args")
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

    logging.info("executing command: {}".format(" ".join(cmd)))
    if secret_arg:
        cmd[secret_arg_ix] = secret_arg

    code = subprocess.call(cmd)

    exit(code)
