#!/usr/bin/env python3

import os
import sys

from ycinfra import (
    get_os_codename,
    write_file_content,
)

REPOS = {
    "xenial": {
        "yandex-cloud-upstream-xenial-secure": {"branch": ["stable"], "arch": ["all", "amd64"]},
        "yandex-cloud-secure": {"branch": ["stable", "prestable", "testing"], "arch": ["all", "amd64"]},
        "yandex-cloud-common-secure": {"branch": ["stable", "prestable", "testing"], "arch": ["all", "amd64"]},
        "yandex-cloud-xenial-esm-secure": {"branch": ["stable", "prestable", "testing"], "arch": ["all", "amd64"]}
    },
    "focal": {
        "yandex-cloud-upstream-focal-secure": {"branch": ["stable"], "arch": ["all", "amd64"]},
        "yandex-cloud-focal-secure": {"branch": ["stable", "prestable", "testing"], "arch": ["all", "amd64"]},
        "yandex-cloud-common-focal-secure": {"branch": ["stable", "prestable", "testing"], "arch": ["all", "amd64"]},
    }
}


def _get_dist_url():
    return os.environ.get('DIST_URL', "dist.yandex.ru")


def unnecessary_repos_removed():
    print("Removing unnecessary repos")
    unnecessary_repo_files = [
        "/etc/apt/sources.list.d/common-stable.list",
        "/etc/apt/sources.list.d/yandex-cloud-amd64-stable.list",
        "/etc/apt/sources.list.d/yandex-cloud-amd64-testing.list",
        "/etc/apt/sources.list.d/yandex-cloud-stable.list",
        "/etc/apt/sources.list.d/yandex-cloud-testing.list",
    ]
    for repo_file in unnecessary_repo_files:
        try:
            print("Removing '{}'".format(repo_file))
            os.remove(repo_file)
        except FileNotFoundError:
            print("File '{}' not found".format(repo_file))
        except PermissionError as ex:
            print("Cannot remove repo: {}".format(ex))
            return False
    return True


def default_repos_cleaned():
    print("Clean 'sources.list' file")
    repo_files = ["/etc/apt/sources.list"]
    for repo_file in repo_files:
        try:
            with open(repo_file, "w"):
                pass
        except (OSError, PermissionError) as ex:
            print("Cannot clean-up 'sources.list' file: {}".format(ex))
            return False
    return True


def generate_repos(repos):
    if not repos:
        return None
    generated = {}
    # Examples:
    # /etc/apt/sources.list.d/yandex-cloud-secure-amd64-stable.list
    # /etc/apt/sources.list.d/yandex-cloud-secure-stable.list
    repo_file_path_template = {
        "amd64": "/etc/apt/sources.list.d/{repo_name}-{arch}-{branch}.list",
        "all": "/etc/apt/sources.list.d/{repo_name}-{branch}.list",
    }
    dist_url = _get_dist_url()
    try:
        for repo_name, repo_properties in repos.items():
            if not isinstance(repo_properties["branch"], list):
                raise TypeError("'branch' should be type list, not {}".format(type(repo_properties["branch"])))
            if not isinstance(repo_properties["arch"], list):
                raise TypeError("'arch' should be type list, not {}".format(type(repo_properties["arch"])))
            for branch in repo_properties["branch"]:
                for arch in repo_properties["arch"]:
                    repo_file_path = repo_file_path_template[arch].format(
                        repo_name=repo_name,
                        arch=arch,
                        branch=branch
                    )
                    # Example:
                    # deb https://yandex-cloud-secure.dist.yandex.ru/yandex-cloud-secure stable/all/
                    repo_string = "deb https://{repo_name}.{dist_url}/{repo_name} {branch}/{arch}/\n".format(
                        repo_name=repo_name,
                        dist_url=dist_url,
                        branch=branch,
                        arch=arch,
                    )
                    generated[repo_file_path] = repo_string
    except (ValueError, TypeError, KeyError) as ex:
        print("Wrong REPOS format: {}".format(ex))
        return None
    return generated


def generated_repos_written(repos):
    for repo_file_path, repo_string in repos.items():
        try:
            write_file_content(repo_file_path, repo_string)
        except OSError as ex:
            print(ex)
            return False
        print("Generated '{}' repo successfully saved".format(repo_file_path))
    return True


if __name__ == "__main__":
    os_codename = get_os_codename()
    if not os_codename:
        print("Cannot define OS codename")
        sys.exit(1)

    if not REPOS.get(os_codename):
        print("Repos are not defined for '{}' distrib".format(os_codename))
        sys.exit(1)

    if not unnecessary_repos_removed():
        sys.exit(1)

    if not default_repos_cleaned():
        sys.exit(1)

    print("Setting repos for '{}' distrib".format(os_codename))
    generated_repos = generate_repos(REPOS.get(os_codename))
    if not generated_repos:
        print("No repos could be generated")
        sys.exit(1)

    if not generated_repos_written(generated_repos):
        sys.exit(1)
