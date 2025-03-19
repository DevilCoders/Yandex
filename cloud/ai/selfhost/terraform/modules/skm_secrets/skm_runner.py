import os
import sys
import tempfile
import subprocess
import json


def create_iam_token(profile):
    result = subprocess.run(
            ['yc', '--profile={}'.format(profile), 'iam', 'create-token'],
            capture_output=True,
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
        skm_env["YAV_OAUTH_TOKEN"] = input['yav_token']
        skm_env["YC_OAUTH_TOKEN"] = create_iam_token(input['profile'])
        result = subprocess.run(
                ["{0} encrypt-md --config {1}".format(input['skm_path'], path)],
                env=skm_env,
                capture_output=True,
                encoding='utf-8',
                shell=True
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
