import logging
import subprocess
import sys

from abc import ABC, abstractmethod
from datetime import datetime

from antiadblock.tasks.tools.logger import create_logger

logger = create_logger("automerge_adblock_extensions")


def run_command(command):
    logger.info("Running command: %s", command)
    try:
        subprocess.check_output(command, shell=True)
    except subprocess.CalledProcessError as e:
        logging.error("Command %s failed, here its output:\n%s", command, e.output, e.stderr)
        raise


class BaseUpdater(ABC):
    @property
    @abstractmethod
    def name(self):
        pass

    @property
    @abstractmethod
    def upstream_repo(self):
        pass

    @property
    def has_submodules(self):
        return False

    def update(self):
        arc_blocker_path = f"./arcadia/antiadblock/blockers/{self.name}"

        run_command(f"git clone {self.upstream_repo} /tmp/{self.name}")

        if self.has_submodules:
            run_command(f"git -C /tmp/{self.name} submodule init")
            run_command(f"git -C /tmp/{self.name} submodule update")

        run_command(f"git -C /tmp/{self.name} "
                    "diff --binary --submodule=diff "
                    f"\"$(cat {arc_blocker_path}/last-commit.sha1)\" HEAD | "
                    f"git --git-dir=/tmp/{self.name}/.git -C {arc_blocker_path} apply --verbose")
        run_command(f"git -C /tmp/{self.name} rev-parse HEAD > {arc_blocker_path}/last-commit.sha1")


class UblockUpdater(BaseUpdater):
    name = "ublock"
    upstream_repo = "https://github.com/gorhill/uBlock.git"
    has_submodules = True


UPDATERS = [
    UblockUpdater()
]


def main():
    branch_name = "update_blockers_" + datetime.now().strftime("%d-%m-%Y")

    run_command("arc mount ./arcadia")
    run_command("cd ./arcadia && "
                f"arc branch {branch_name} trunk && "
                f"arc checkout {branch_name}")

    successful_updates = []
    failed_updates = []

    for updater in UPDATERS:
        try:
            updater.update()
            successful_updates.append(updater.name)
        except Exception as e:
            logger.info(e)
            failed_updates.append(updater.name)

    if successful_updates:
        run_command("cd ./arcadia && "
                    "arc add . && "
                    "arc commit -m 'Update blockers' && "
                    "arc push --force && "
                    "arc pr create --publish -m 'Update blocker'")

    if failed_updates:
        logger.error("Failed to update: {}".format(', '.join(failed_updates)))
        sys.exit(-1)


if __name__ == "__main__":
    main()
