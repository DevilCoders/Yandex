"""
Backup monitoring
"""

import os
import json
import logging
from pprint import pformat
from datetime import datetime, timedelta
from collections import defaultdict
from dotmap import DotMap

class BackupMonitoring(object):
    """Check s3 backups"""

    log = logging.getLogger(__name__)

    def __init__(self, conf):
        self.conf = conf
        self.bconf = conf.buckets[conf.current_bucket]
        if self.conf.debug:
            console = logging.StreamHandler()
            console.setLevel(logging.DEBUG)
            logging.getLogger().addHandler(console)

        self.stats_mtime = datetime.now()

    def run(self, zk_cli):
        """Run checks"""
        self.log.info("Work with bucket: %s", self.conf.current_bucket)
        monitoring_directory = os.path.join(
            zk_cli.conf.zk.basepath, self.conf.current_bucket
        )
        mon_file = os.path.join(
            monitoring_directory, self.conf.check_for_database
        )
        stats = zk_cli.client.exists(mon_file)
        if not stats:
            print("2;Zk node not found {}, run rotation before checking".format(mon_file))
            os.sys.exit(0)

        self.stats_mtime = datetime.fromtimestamp(stats.last_modified)
        if datetime.now().timestamp() - stats.last_modified > 26 * 3600: # 26 hours
            print("2;Zk rotation stats {} too old: {}".format(mon_file, self.stats_mtime))
            os.sys.exit(0)

        mon_data = self.load_data_from_zk(zk_cli, mon_file)
        mon_message = self.check(mon_data)
        zk_cli.write(self.conf.zk_last_status, mon_message.encode())
        print(mon_message)

        self.log.info("Done for bucket: %s", self.conf.current_bucket)


    def load_data_from_zk(self, zk_cli, mon_file):
        """Load backups stats from zk"""

        rotated = range(1, self.conf.monitoring.rotate+1)

        # файлы с предыдущего сбора данных мониторинга ротируются и сохранаяются
        # под номерами file.1, file.2, ..., file.<rotate_num>
        # несколько файлов нужно для того, чтобы более точно мониторить размер бекапа.
        # Размер бекапа рассчитывается относительно среднего
        # из всех файлов с данными мониторинга
        mon_files = [mon_file] + list("{}.{}".format(mon_file, i) for i in rotated)

        backup_stats = []
        for mfile in mon_files:
            if zk_cli.client.exists(mfile):
                value, _ = zk_cli.client.get(mfile)
                db_data = json.loads(value.decode())
                # если в zk какой-то хлам, тоже пропускаем его
                if db_data and isinstance(db_data, dict):
                    backup_stats.append(db_data)
        return backup_stats


    def check(self, backup_stats):
        """Perform the check and monrun output"""
        policy = self.conf.policy.copy()
        policy.update(self.conf.buckets[self.conf.current_bucket].policy)
        policy.update(self.bconf[self.conf.check_for_database].policy)

        missing = defaultdict(list)
        db_stats = backup_stats[0]
        for host, backups in db_stats.items():
            if self.bconf.daily_only:
                missing_count = self.check_daily_only(backups)
            else:
                missing_count = sum(policy.values()) - len(backups)

            if missing_count > 0:
                missing["Missing {} backups".format(missing_count)].append(host)
        if missing:
            return "2;" + self.format_msg(missing)

        # далее проверка размера бекапа
        suspicious_size = self.check_size(backup_stats)
        if suspicious_size:
            return "2;" + self.format_msg(suspicious_size)

        return "0;Ok"


    def check_daily_only(self, backups):
        """
        Если дневных бекапов очень много и хочется мониторить
        только их поступление, то проверяем наличие последнего бекапа,
        а затем от последнего наличие всех остальных до daily_only
        """
        days_limit = int(self.bconf.daily_only)
        missing_count = 0
        # от самого свежего бекапа идем к самому старому
        backups = sorted(backups, key=lambda x: x["path"], reverse=True)
        days_check = 0
        for backup in backups:
            # нам нужна строка в формате YYYYMMDD но в mysql строка имеет формат
            # YYYY-MM-DD поэтому придется реплейсить '-'
            date_have = backup["path"].strip("/").split("/")[-1].replace("-", "")

            date = self.stats_mtime - timedelta(days=days_check)
            date_want = date.strftime("%Y%m%d")
            while date_have != date_want:
                # если предполагаемая дата не равна текущей, вероятно это
                # пропущенный бекап, нужно проверять дальше, пока не найдем
                days_check += 1
                missing_count += 1
                date = self.stats_mtime - timedelta(days=days_check)
                date_want = date.strftime("%Y%m%d")
            if days_check >= days_limit:
                break
            days_check += 1

        if days_limit > days_check:
            # если удалось проверить меньше бекапов, чем задано через daily_only
            # считаем, что остальных бекапов нет
            missing_count += days_limit - days_check

        return missing_count


    def check_size(self, stats):
        """Check size difference for backups"""

        size_stats = defaultdict(lambda: DotMap({
            "date_set": set(), "backups": list(),
            "sum_size": 0, "total_objects": 0,
            "penult_size": 0, "penult_objects": 0,
            "penult_date": 0, "count": 0
        }))

        # stats это массив из диктов, каждый дикт в формате
        # {"hostname": [backup1, backup2, ..., backupN]}
        # где backupN - это BackupItem сериализованный в dict
        # для кассандры hostname - это честный hostname
        # для монги - это имя шрда а для mysql - это псевдоним
        # смотрите mysql-backup-data чтобы понять как он формируется
        for db_stats in stats:
            self.collect_size_stats(size_stats, db_stats)

        suspicious_size = defaultdict(list)
        for name, bkp in size_stats.items():
            if bkp.count == 0: # paranoic check
                self.log.error("No backups for %s", name)
                continue

            if name in self.bconf.skip_size_check:
                self.log.debug("Skip size check for %s", name)
                continue

            mon_conf = self.conf.monitoring
            # default 10 % max size diff
            limit_down = mon_conf.max_size_diff or self.bconf.max_size_diff or 10
            limit_up = mon_conf.max_size_diff_up or self.bconf.max_size_diff_up or 50

            size_percent = bkp.penult_size / (bkp.sum_size / bkp.count / 100.0)
            bkp.size_diff = 100 - size_percent

            # backup less then usual
            if bkp.size_diff > limit_down:
                check_message = "{} smaller than usual by {:0.1f}%".format(
                    bkp.penult_date, bkp.size_diff
                )
                suspicious_size[check_message].append(name)
            elif abs(bkp.size_diff) > limit_up:
                # backup bigger then usual
                check_message = "{} larger than usual by {:0.1f}%".format(
                    bkp.penult_date, abs(bkp.size_diff)
                )
                suspicious_size[check_message].append(name)

            if self.conf.debug:
                self.log.debug(pformat(bkp.toDict()))
        return suspicious_size


    def collect_size_stats(self, container, db_stats):
        """Collect total backups size and object counts"""
        for host, backups in db_stats.items():
            for idx, backup in enumerate(
                    sorted(backups, key=lambda x: x["path"], reverse=True)
            ):
                if idx == 0:
                    continue

                backup = DotMap(backup)
                date = backup.path.strip("/").split("/")[-1].replace("-", "")
                if date in container[host].date_set:
                    continue

                if self.bconf.check_size_last_days:
                    delta = datetime.now() - datetime.strptime(date, '%Y%m%d')
                    if abs(delta.days) > self.bconf.check_size_last_days:
                        continue

                container[host].date_set.add(date)

                container[host].backups.append(backup)
                if idx == 1:
                    container[host].dbtype = self.conf.check_for_database
                    container[host].penult_size = backup.size
                    container[host].penult_objects = backup.objects
                    container[host].penult_date = date
                else:
                    container[host].sum_size += backup.size
                    container[host].total_objects += backup.objects
                    container[host].count += 1



    def get_last_status(self, zk_cli):
        """Get last backup check status"""
        if zk_cli.client.exists(self.conf.zk_last_status):
            val, _ = zk_cli.client.get(self.conf.zk_last_status)
            return val.decode()
        return "1;Unknown status"

    @staticmethod
    def format_msg(data):
        """Format monitorring message"""
        return ", ".join(
            "{}: {}".format(k, ",".join(v)) for k, v in data.items()
        )
