"""S3 client lib"""

import logging
import subprocess
from .item import BackupItem

class S3Cli():
    """S3 client"""

    log = logging.getLogger(__name__)

    def __init__(self, cfg):
        self.s3cfg = cfg

    def s3rm(self, backup):
        """s3rm remove backup on s3 storage"""
        # paranoic mode on
        parts = backup.stats.path.strip("/").split("/")
        parts_len = len(parts)
        if parts_len < 7:
            self.log.error("Failed to remove backup %s, path too short", backup)
            return
        # paranoic mode off
        try:
            content = subprocess.check_output(
                ["s3cmd", "--config", self.s3cfg, "rm", "--recursive", backup.stats.path]
            )
            self.log.info("%s", content)
        except (subprocess.CalledProcessError, OSError) as exc:
            self.log.error("Failed to remove backup %s: %s", backup, exc)


    def s3du(self, backup):
        """Check backup size"""
        try:
            content = subprocess.check_output(
                ["s3cmd", "--config", self.s3cfg, "du", backup.stats.path]
            ).strip().split()
            backup.stats.size = int(content.pop(0))
            backup.stats.objects = int(content.pop(0))
        except (IndexError, ValueError, subprocess.CalledProcessError, OSError) as exc:
            self.log.error("Failed to check backup %s: %s", backup, exc)


    def s3ls(self, s3path):
        """List s3 path"""
        if not s3path.endswith("/"):
            s3path = s3path + "/"

        try:
            self.log.info("List %s", s3path)
            content = subprocess.check_output(
                ["s3cmd", "--config", self.s3cfg, "ls", s3path]
            ).splitlines()
        except (subprocess.CalledProcessError, OSError) as exc:
            self.log.error("Failed to list %s: %s", s3path, exc)
            content = []

        dirs = []
        stop_descent = False
        backups_list = []
        for line in content:
            line = line.decode().strip()
            atype, s3path = line.rsplit(None, 1)
            if atype != "DIR":
                continue

            item = BackupItem(path=s3path)
            if item.matched:
                self.log.debug("Found backup: %s", item)
                stop_descent = True
                backups_list.append(item)
            else:
                self.log.debug("Found directory '%s'", line)
                dirs.append(s3path)

        if not stop_descent:
            self.log.info("Descent subdirs %s", s3path)
            for dirname in dirs:
                backups_list.extend(self.s3ls(dirname))
        return backups_list


    def purge_stale(self, backups_list, dry_run=False):
        """Purge stale backups"""
        for backup in backups_list:
            if not backup.valid:
                self.log.info("Remove stale backup %s", backup)
                if dry_run:
                    self.log.info("Dry run, skip 'self.s3rm(backup)'")
                else:
                    self.s3rm(backup)
            else:
                self.log.debug("Preserve backup %s", backup)
