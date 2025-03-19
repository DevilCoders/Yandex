#! /usr/bin/env python3


import os


def create_pid_file(pid_file_path):
    if not os.path.isfile(pid_file_path):
        with open(pid_file_path, "w") as pid_file:
            pid_file.write(str(os.getpid()))
    return True


def delete_pid_file(pid_file_path):
    if os.path.isfile(pid_file_path):
        os.remove(pid_file_path)
