#!/usr/bin/env python3
import argparse
import os
import subprocess


def get_size(start_path):
    total_size = 0
    for dirpath, dirnames, filenames in os.walk(start_path):
        for f in filenames:
            fp = os.path.join(dirpath, f)
            # skip if it is symbolic link
            if not os.path.islink(fp):
                total_size += os.path.getsize(fp)

    return total_size


def run_commands_sequence(commands):
    exec_env = os.environ.copy()
    exec_env["LIBGUESTFS_BACKEND"] = "direct"
    full_command_sequence = '\n'.join([' '.join(cmd) for cmd in commands])
    print('Run following command sequence:\n' + full_command_sequence)
    for cmd in commands:
        print('Run: ' + ' '.join(cmd))
        res = subprocess.run(cmd, capture_output=True, env=exec_env)
        print(res.stdout.decode("utf-8"))
        print(res.stderr.decode("utf-8"))
        if res.returncode != 0:
            print('ERROR: Command was not finished successfully, return code:', res.returncode)
            return res.returncode


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--data-path', help='path to directory with files')
    parser.add_argument('-o', '--output-file', help='path to output file')
    parser.add_argument('-s', '--size', help='size of resulting image')
    args = parser.parse_args()

    image_raw_name = 'output.img'
    image_qcow2_name = args.output_file
    size_argument = [] 
    if args.size is not None:
        size_argument = ["--size={}".format(args.size)]

    run_commands_sequence(
        [
            ['virt-make-fs', '-v', '--format=qcow2', '--type=ext3'] + size_argument + [args.data_path, image_qcow2_name]
        ]
    )


if __name__ == '__main__':
    main()
