import boto3
import datetime
import os
import sys
import subprocess
import json

from json import JSONDecodeError
from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger


class TargetWrapper:
    def __init__(self, target, stdout_log_file):
        self.target = target
        self.stdout_log_file = stdout_log_file
        self.exit_message = None

    def __call__(self, *args, **kwargs):
        try:
            ThreadLogger.register_thread(self.stdout_log_file)
            self.target(*args, **kwargs)
        except Exception as e:
            self.exit_message = repr(e)


class HttpCode:
    OK = 200
    BAD_REQUEST = 400
    FORBIDDEN = 403
    UNAVAILABLE = 503


def template_job_output_json(exit_code, current_status, stderr_head):
    return {
        "failure": "JOB_LAUNCHER",
        "task": "PREPARE",
        "exit_status": exit_code,
        "custom_message": f'Job failed on step {current_status}, head of stderr: {stderr_head}'
    }


def get_job_launcher_stderr_json(exit_code, current_status, process_stderr):
    try:
        # get first line of output with job_launcher stderr
        job_launcher_output = json.loads(process_stderr[0])
    except IndexError:
        # stderr is empty list
        job_launcher_output = template_job_output_json(exit_code, current_status, "")
    except JSONDecodeError:
        stderr_head = '\n'.join(process_stderr[:10])
        job_launcher_output = template_job_output_json(exit_code, current_status, stderr_head)

    return job_launcher_output


def get_s3_client(endpoint_url, aws_access_key, aws_secret_key):
    session = boto3.session.Session(
        aws_access_key_id=aws_access_key,
        aws_secret_access_key=aws_secret_key
    )
    return session.client(
        service_name='s3',
        endpoint_url=endpoint_url
    )


def read_s3_credentials(path):
    credentials = {
        'aws_access_key': '',
        'aws_secret_key': ''
    }
    with open(path, 'r') as f:
        key, value = f.readline()[:-1].split('=')
        assert key in credentials.keys()
        credentials[key] = value

        key, value = f.readline()[:-1].split('=')
        assert key in credentials.keys()
        credentials[key] = value

    return credentials


def current_timestamp():
    return datetime.datetime.now().isoformat()[:-3] + 'Z'


def read_file_to_str(file):
    if file is None:
        return ''
    with open(file, 'r') as f:
        return f.readlines()


def print_error(message, code=1):
    sys.stderr.write("Error: {__message}\n".format(__message=message))


def check_return_code(code):
    if code != 0:
        print_error("Subprocess returned non-zero exit code {__exit_code}".format(__exit_code=code), code)


def call_subprocess_with_check(command, shell=True, env=os.environ):
    check_return_code(subprocess.call(command, shell=shell, env=env))


def is_empty_file(path):
    return os.stat(path).st_size == 0


def merge_layers(layers):
    call_subprocess_with_check("rm -rf data", shell=True)
    call_subprocess_with_check("rm result.tar.gz", shell=True)
    call_subprocess_with_check("mkdir data", shell=True)

    for layer_path in layers:
        if not is_empty_file(layer_path):
            # decompress *.tar.xz
            call_subprocess_with_check(
                "tar -xf {__path} -C data/".format(__path=layer_path),
                shell=True)

    call_subprocess_with_check("cd data; tar czf ../{__output_archive} .; cd .."
                               .format(__output_archive="result.tar.gz"), shell=True)


def prepare_archive(layers, tmp_dir):
    untarred_layers_dir = os.path.join(tmp_dir, 'untarred_layers')
    result_archive_file = os.path.join(tmp_dir, 'merged_layers.tar.gz')
    args = [f'mkdir -p {untarred_layers_dir}']

    for layer_path in layers:
        # decompress *.tar.xz
        args.append(f'tar -xf {layer_path} -C {untarred_layers_dir}')

    args.append(f'tar -C {untarred_layers_dir} -czf {result_archive_file} .')

    return "; ".join(args), result_archive_file


def merge_layers_args(layers, layers_dir, docker_base_image_name):
    tmp_dir = os.path.join(layers_dir, 'tmp')
    prepare_archive_command, result_archive_file = prepare_archive(layers, tmp_dir)
    args = [prepare_archive_command, f'docker import  - {docker_base_image_name} < {result_archive_file}',
            f'rm -rf {tmp_dir}']

    return "; ".join(args)

