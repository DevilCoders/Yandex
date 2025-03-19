import os
import sys
import tempfile
import subprocess
import json
from subprocess import PIPE


def create_iam_token():
    result = subprocess.run(
            ['yc', 'iam', 'create-token'],
            stdout=PIPE, stderr=PIPE,
            encoding='utf-8')
    result.check_returncode()
    return result.stdout


def create_yav_token():
    result = subprocess.run(
        ['ya', 'vault', 'oauth'],
        stdout=PIPE, stderr=PIPE,
        encoding='utf-8')
    result.check_returncode()
    return result.stdout


def main():
    input = json.load(sys.stdin)
    fd, path = tempfile.mkstemp()
    contents = json.loads(input['files'])
    try:
        for content_desc in contents:
            filename = content_desc['source_path']
            os.makedirs(os.path.dirname(filename), exist_ok=True)
            f = open(filename, 'w')
            f.write(content_desc['content'])
            f.close()

        with os.fdopen(fd, 'w') as tmp:
            tmp.write(input['src'])

        skm_env = os.environ.copy()
        skm_env["YAV_OAUTH_TOKEN"] = input['yav_token'] or create_yav_token()
        skm_env["YC_OAUTH_TOKEN"] = os.getenv("YC_OAUTH_TOKEN") or os.getenv("YC_TOKEN") or create_iam_token()
        result = subprocess.run(
                ["{0} encrypt-md --config {1}".format(input['skm_path'], path)],
                env=skm_env,
                stdout=subprocess.PIPE,
                encoding='utf-8',
                shell=True,
        )
        result.check_returncode()
        dst = result.stdout
        if not dst.startswith('skm:'):
            raise ValueError(dst)
        output = {
            "dst": "\n".join(dst.split("\n")[1:])
        }
        print(json.dumps(output))
    finally:
        os.remove(path)
        for content_desc in contents:
            filename = content_desc['source_path']
            if os.path.exists(filename):
                os.remove(filename)


if __name__ == '__main__':
    main()
