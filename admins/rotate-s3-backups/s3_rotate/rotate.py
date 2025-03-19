"""Backup rotate"""

import os
import math
import logging
import json
from datetime import datetime, timedelta
from collections import defaultdict
from .s3_client import S3Cli
from .item import BackupItem

class BackupRotate():
    """Rotate s3 backups"""

    log = logging.getLogger(__name__)

    def __init__(self, conf):
        self.backups_list = list()
        self.dbtype = None
        self.strict = True
        self.conf = conf
        self.stats = dict()
        self.s3cli = S3Cli(conf.buckets[conf.current_bucket].s3cfg)


    def timestamp(self):
        """Get most recent backup timestamp"""
        if self.conf.save_from_last:
            return self.backups_list[-1].timestamp
        return datetime.now()


    @staticmethod
    def get_checkpoin(timestamp, freq, factor):
        """relativedelta by frequency"""
        days = 1 * factor
        if freq == "weekly":
            days = 7 * factor
        if freq == "monthly":
            # tmp.day - contains day of month and delta will be substracted from
            # timestamp, therefore tmp.day + 1 enought to get prev month
            # and repeat it for 'factor' times
            tmp = timestamp
            days = 0
            for _ in range(factor):
                days += tmp.day
                tmp = tmp - timedelta(days=tmp.day)
        new_chkp_timestamp = timestamp - timedelta(days=days)
        chkp = BackupItem(timestamp=new_chkp_timestamp)
        return chkp


    def group_by_frequency(self):
        """Group backup by dates"""
        backup_groups = defaultdict(lambda: defaultdict(lambda: defaultdict(list)))
        self.backups_list = sorted(self.backups_list, key=lambda x: x.timestamp)
        for bkp in self.backups_list:  # pylint: disable=invalid-name
            backup_groups[bkp.stats.name]["daily"][bkp.key("daily")].append(bkp)
            backup_groups[bkp.stats.name]["weekly"][bkp.key("weekly")].append(bkp)
            backup_groups[bkp.stats.name]["monthly"][bkp.key("monthly")].append(bkp)
        return backup_groups


    def select_valid(self, backup_groups, policy):
        """Select actual backups"""
        from_ts = self.timestamp()
        self.log.debug("Current timestamp %s, policy %s", from_ts.strftime("%F"), policy)
        # entity тут будет ходержать имя базы
        # frequencies список списков бекапов, сгруппированных по периодам.
        for entity, frequencies in backup_groups.items():
            for frequency in BackupItem.supported_frequencies:
                backups = frequencies.get(frequency, None)
                if not backups:
                    self.log.debug("No %s backups for %s", frequency, entity)
                    continue
                self.log.debug("Check %s backups for %s", frequency, entity)
                # transition - это бекапы, которые попадают на промежуток
                # между последним дневным и первым из более длинных периодов
                # переходный бекап нужно сохранять для того, чтобы при выборе
                # бекапов в более длинных периодах было из чего выбирать.
                transition = 0
                if frequency == "weekly":
                    transition = math.ceil(policy.daily / 7)
                if frequency == "monthly":
                    transition = math.ceil((policy.daily + policy.weekly * 7) / 30)

                for delta in range(policy.get(frequency, 0) + transition):
                    # chkp указывает на дату, в которой нужно выбирать бекапы
                    # в зависимости от текущего frequency у chkp выбирается либо
                    # день, либо номер недели или месяца, потом из сгруппированных
                    # бекапов берется список для этого периода и самый
                    # новый бекап из этого списка помечается как валидный для сохранения
                    chkp = self.get_checkpoin(from_ts, frequency, delta)

                    bkp_type = "normal"
                    if frequency != "daily" and delta < transition:
                        # явно логируем, что выбираемые бекапы будут переходными
                        # а не сохраненными по policy
                        bkp_type = "transitional"

                    key = chkp.key(frequency)
                    backups_list = backups.get(key, [])
                    self.log.debug(
                        "Select %s backups in %s(chkp=%s)",
                        bkp_type, key, chkp.timestamp.strftime("%F")
                    )

                    if not backups_list:
                        self.log.debug("Not found %s backups in selected period", bkp_type)
                        continue
                    bkp = backups_list[0]
                    if not bkp.valid:
                        self.log.debug("Set valid=1 for %s backup:  %s", bkp_type, bkp)
                        bkp.valid = 1
                        bkp.state = bkp_type


    def rotate_bucket(self, zk_cli):
        """Rotate backups for bucket"""
        bucket = self.conf.current_bucket

        policy = self.conf.policy.copy()
        policy.update(self.conf.buckets[bucket].policy)
        for dbtype in self.conf.buckets[bucket].databases:
            self.log.info("Rotate %s databases in bucket %s", dbtype, bucket)
            prefix = "s3://{0}/backup/{1}/".format(bucket, dbtype)
            prefix = self.conf[bucket][dbtype].basename or prefix
            self.backups_list = list()
            self.dbtype = dbtype
            self.backups_list = self.s3cli.s3ls(prefix)
            policy.update(self.conf.buckets[bucket][dbtype].policy)
            self.select_valid(self.group_by_frequency(), policy)
            self.s3cli.purge_stale(self.backups_list, self.conf.args.dry_run)
            self.write_monitoring_data(zk_cli)
            self.log.info("Rotate %s databases in bucket %s completed", dbtype, bucket)

    def run(self, zk_cli):
        """Run rotation"""
        self.log.info("Work with bucket: %s", self.conf.current_bucket)
        self.rotate_bucket(zk_cli)
        self.log.info("Done for bucket: %s", self.conf.current_bucket)


    def write_monitoring_data(self, zk_cli):
        """Write data for monitoring"""
        monitoring_data = defaultdict(list)
        for backup in self.backups_list:
            if not backup.valid:
                continue
            self.s3cli.s3du(backup)
            self.log.debug("Backup stats %s", backup.stats)
            monitoring_data[backup.stats.name].append({
                "path": backup.stats.path,
                "size": backup.stats.size,
                "objects": backup.stats.objects,
            })
        monitoring_directory = os.path.join(
            zk_cli.conf.zk.basepath, self.conf.current_bucket
        )
        mon_file = os.path.join(monitoring_directory, self.dbtype)
        zk_cli.rotate_mon_file(mon_file)
        if self.conf.args.dry_run:
            self.log.info(
                "Dry run: write monitoring data into zk file %s: %s",
                mon_file, monitoring_data
            )
        else:
            zk_cli.write(mon_file, json.dumps(monitoring_data).encode())
