import os


def get_project_root_dir():
    root_dir = os.path.dirname(__file__)
    return os.path.dirname(root_dir)


def get_project_path(rel_path):
    root_dir = get_project_root_dir()
    return os.path.join(root_dir, rel_path)
