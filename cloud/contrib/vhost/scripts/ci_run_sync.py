#!/usr/bin/python3
import argparse
import subprocess
import requests
from requests.adapters import HTTPAdapter, Retry
import os
import time
import json


BB_LIBVHOST_PROJECT = "CLOUD"
BB_LIBVHOST_REPO = "libvhost-server"
BB_TEST_PR_PREFIX = "bb-test-pr-"


def bb_pr_number():
    pr = os.environ["BUILD_BRANCH"]
    return pr.replace("pull-requests/", "")


def make_oauth_header(token_path):
    with open(token_path, "r") as tk:
        return {
            "Authorization": "OAuth {}".format(tk.read())
        }


class BBApi:
    api_link = "https://bb.yandex-team.ru/rest/api/1.0/projects/{}/repos/{}"
    repo_link = "https://bb.yandex-team.ru/projects/{}/repos/{}"

    def __init__(self, req_session, project, repo, token_path):
        self.__req_session = req_session
        self.__api_link = BBApi.api_link.format(project, repo)
        self.__repo_link = BBApi.repo_link.format(project, repo)
        self.__headers = make_oauth_header(token_path)

    def get_pr_link(self, pr_id):
        return "{}/{}/{}".format(self.__repo_link, "pull-requests",
                                 pr_id)

    def __make_api_link(self, pr_id, postfix=""):
        return "{}/{}/{}{}".format(self.__api_link, "pull-requests",
                                   pr_id, postfix)

    def pr_info(self, pr_id):
        link = self.__make_api_link(pr_id)
        resp = self.__req_session.get(link, headers=self.__headers)

        # This is valid in case the PR was deleted
        if resp.status_code == 404:
            return None

        resp.raise_for_status()
        return resp.json()

    def post_comment(self, pr_id, text):
        link = self.__make_api_link(pr_id, "/comments")
        resp = self.__req_session.post(link, json={"text": text},
                                       headers=self.__headers)
        resp.raise_for_status()


class ArcApi:
    def __init__(self, req_session, token_path):
        self.__req_session = req_session
        self.__headers = make_oauth_header(token_path)

    @staticmethod
    def get_pr_link(pr_id):
        return "https://a.yandex-team.ru/review/{}".format(pr_id)

    @staticmethod
    def __make_api_link(pr_id, method):
        link = "https://a.yandex-team.ru/api/review/review-request/{}/{}"
        return link.format(pr_id, method)

    def wait_on_ci_checks(self, pr_id) -> bool:
        link = ArcApi.__make_api_link(pr_id, "commit-button")
        res = True

        while True:
            resp = self.__req_session.get(link, headers=self.__headers)
            resp.raise_for_status()

            ci_checks = resp.json()["result"]["ciCheck"]
            statuses = [
                ci_checks["buildStatus"],
                ci_checks["testsStatus"],
                ci_checks["largeTestsStatus"]
            ]

            if any(sts == "failed" for sts in statuses):
                res = False
                break

            if not any(sts == "in_progress" for sts in statuses):
                break

            print("Waiting for the CI checks to finish...", flush=True)
            time.sleep(60 * 5)

        return res


def run_sync(path, arc_path, description, sync_branch):
    args = [path, arc_path, "--arc-sync-branch", sync_branch]

    if description is not None:
        args.extend(["--pr-description", description])

    subprocess.check_call(args, timeout=30)


def get_new_pr_description(bb_pr_id, bb_api: BBApi):
    pr_info = bb_api.pr_info(bb_pr_id)
    bb_pr_link = bb_api.get_pr_link(pr_info["id"])
    pr_desc_template = "[BB-libvhost] {}\n\n{}\nOriginal bitbucket PR: {}"

    return pr_desc_template.format(pr_info["title"],
                                   pr_info.get("description", ""),
                                   bb_pr_link)


class Arc:
    def __init__(self, path):
        self.__path = path

    @property
    def path(self):
        return self.__path

    def __arc_check_output(self, cmd):
        cmd = ["arc", *cmd]
        out = subprocess.check_output(cmd, cwd=self.__path,
                                      universal_newlines=True)
        return out.strip("\n")

    def __arc_check_json_output(self, cmd):
        out = self.__arc_check_output([*cmd, "--json"])
        if not out:
            return None

        return json.loads(out)

    def __arc_check_call(self, cmd):
        subprocess.check_output(["arc", *cmd], cwd=self.__path)

    def pr_status(self, pr_id=None):
        args = ["pr", "status"]
        if pr_id is not None:
            args.append(str(pr_id))

        return self.__arc_check_json_output(args)

    def pr_list(self):
        out = []

        # Can't use check_json_output because 'pr list' returns
        # invalid json that we have to fixup manually before
        # attempting to parse it.
        res = self.__arc_check_output(["pr", "list", "--json"])
        if not res:
            return out

        # Fixup invalid json returned by arc manually
        res = res.replace("}\n", "},")
        res = "[" + res + "]"
        prs = json.loads(res)

        for pr in prs:
            out.append(self.pr_status(pr["id"]))

        return out

    def pr_close(self, pr_id):
        self.__arc_check_call(["pr", "discard", str(pr_id)])


def reap_orphan_arc_prs(arc: Arc, bb_api: BBApi):
    prs = arc.pr_list()

    for pr in prs:
        arc_branch = pr["branch"]
        br_parts = arc_branch.split("/", 2)

        if len(br_parts) != 3 or not br_parts[2].startswith(BB_TEST_PR_PREFIX):
            print("Ignoring PR from branch: '{}'".format(arc_branch))
            continue

        bb_pr_id = br_parts[2].replace(BB_TEST_PR_PREFIX, "")
        if not bb_pr_id.isdigit():
            print("Ignoring invalid arc branch: '{}'".format(arc_branch))
            continue

        bb_pr_info = bb_api.pr_info(bb_pr_id)

        # Handle 404 manually
        if bb_pr_info is None:
            print("Couldn't find BB PR {}, assuming it was deleted"
                  .format(bb_pr_id))

            bb_pr_info = {
                "closed": True,
                "state": "DELETED"
            }

        if bb_pr_info["closed"]:
            print("Closing arcadia PR {} because BB PR {} is in state '{}'"
                  .format(pr["id"], bb_pr_id, bb_pr_info["state"]))
            arc.pr_close(pr["id"])


def main():
    parser = argparse.ArgumentParser(
        "CI only! Run bb->arc sync and cleanup orphan PRs")
    parser.add_argument("sync_script_path", help="Path to the sync script")
    parser.add_argument("arc_path", help="Path to the arcardia repository")
    parser.add_argument("git_path", help="Path to the git repository")
    parser.add_argument("bb_token_path", help="Path to the BB token file")
    parser.add_argument(
        "arc_token_path",
        help="Path to the arcadia token file")
    args = parser.parse_args()

    req_session = requests.Session()
    retries = Retry(total=5, backoff_factor=1,
                    status_forcelist=[500, 502, 503, 504])
    req_session.mount("http://", HTTPAdapter(max_retries=retries))
    req_session.mount("https://", HTTPAdapter(max_retries=retries))

    arc = Arc(args.arc_path)
    bb_api = BBApi(req_session, BB_LIBVHOST_PROJECT, BB_LIBVHOST_REPO,
                   args.bb_token_path)
    arc_api = ArcApi(req_session, args.arc_token_path)

    bb_pr_id = bb_pr_number()
    sync_branch = BB_TEST_PR_PREFIX + bb_pr_id

    prs = arc.pr_list()
    resync = False

    for pr in prs:
        if pr["branch"].endswith(sync_branch):
            resync = True
            break

    pr_description = None
    if not resync:
        pr_description = get_new_pr_description(bb_pr_id, bb_api)

    run_sync(args.sync_script_path, arc.path, pr_description, sync_branch)

    this_id = arc.pr_status()["id"]
    if not resync:
        arc_pr_link = ArcApi.get_pr_link(this_id)

        text = "Created a new arcanum PR: {}.\n".format(arc_pr_link) + \
               "Please wait for the CI checks to finish, " \
               "this may take a while.\n" \
               "You will be notified if anything goes wrong.\n"

        bb_api.post_comment(bb_pr_id, text)

    print("Arcanum PR: {}".format(arc_pr_link))

    # Wait a bit for the CI checks to appear/reset
    time.sleep(120)

    if not arc_api.wait_on_ci_checks(this_id):
        message = "An arcanum CI check has failed!\n" \
                  "Please investigate and rerun the PR " \
                  "check once you're done.\n"

        bb_api.post_comment(bb_pr_id, message)
        exit(1)

    reap_orphan_arc_prs(arc, bb_api)


if __name__ == "__main__":
    main()
