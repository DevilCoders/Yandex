#!/usr/bin/env python3

import os
import json
import logging
import time
import subprocess
from yc_monitoring import JugglerPassiveCheck, JugglerPassiveCheckException

__author__ = "Igor Miheenko"
__copyright__ = "Copyright 2017, Yandex LLC"
__email__ = "affe@yandex-team.ru"

logging.basicConfig(format="[%(asctime)s] %(levelname)-8s %(filename)s:%(lineno)-3d> (%(funcName)s) %(message)s",
                    # level=logging.DEBUG)
                    )
log = logging.getLogger(__name__)


class Checker(object):
    def __init__(self):
        super(Checker, self).__init__()

    def get_ext4_partitions(self):
        _ext4_list = []
        _ext4_raw_list = []
        _filename = "/proc/mounts"
        with open(_filename, "r") as f:
            _raw_data = f.read()
        _raw_data_list = _raw_data.split("\n")
        _raw_data_list.pop()
        for row in _raw_data_list:
            row_list = row.split()
            if row_list[2] == "ext4":
                _ext4_raw_list.append(row_list[0])
        log.debug("_ext4_raw_list = {}".format(_ext4_raw_list))
        #
        if _ext4_raw_list:
            for part in _ext4_raw_list:
                _part_list = part.split("/")
                if _part_list[-2] == "by-uuid":
                    try:
                        _raw_part = os.readlink(part)
                        _ext4_list.append(_raw_part.split("/")[-1])
                    except OSError as e:
                        log.debug(e)
                        _ext4_list.append(self.run_blkid(_part_list[-1]))
                elif not _part_list[-1][:4] == "loop":
                    _ext4_list.append(_part_list[-1])
        log.debug("ext4_list = {}".format(_ext4_list))
        return _ext4_list

    @staticmethod
    def run_blkid(pattern):
        log.debug("pattern = {}".format(pattern))
        _part = None
        _raw_output = None
        _command = ["blkid", "-U", pattern]
        try:
            _raw_output = subprocess.check_output(_command)
        except subprocess.CalledProcessError as e:
            log.debug(e)
        if _raw_output:
            _part_full = _raw_output.strip().decode()
            _part = _part_full.split("/")[-1]
        log.debug("_part = {}".format(_part))
        return _part

    @staticmethod
    def read_errors(ext4_list):
        count_dict = {}
        for part in ext4_list:
            _path = "/sys/fs/ext4/{}/errors_count".format(part)
            try:
                with open(_path, "r") as f:
                    raw_data = f.read()
            except IOError as e:
                raw_data = "-1"
                log.debug(e)
            errors_count = raw_data.split()[0]
            count_dict.update({part: int(errors_count)})
        log.debug("count_dict = {}".format(count_dict))
        return count_dict

    @staticmethod
    def generate_dict(count_dict):
        threshold = 10
        device_list = []
        crit_errors = 0
        warn_errors = 0
        for part, count in count_dict.items():
            if count >= threshold:
                crit_errors += 1
                msg = "partition {} errors count {} is greater than {} threshold".format(part, count, threshold)
                device_list.append({"name": part, "error_count": count, "threshold": threshold, "message": msg})
            elif 0 < count < threshold:
                warn_errors += 1
                msg = "partition {} errors count {} is less than {} threshold".format(part, count, threshold)
                device_list.append({"name": part, "error_count": count, "threshold": threshold, "message": msg})
            elif count == -1:
                warn_errors += 1
                msg = "cannot read errors_count on {};".format(part)
                device_list.append({"name": part, "error_count": count, "threshold": threshold, "message": msg})
            else:
                msg = "errors not found"
                device_list.append({"name": part, "error_count": count, "threshold": threshold, "message": msg})
        output_msg = json.dumps({"result": {"device_list": device_list}, "timestamp": time.time()})
        if crit_errors:
            exit_num = 2
        elif not crit_errors and warn_errors:
            exit_num = 1
        else:
            exit_num = 0
        return exit_num, output_msg

    def run(self, check: JugglerPassiveCheck):
        ext4_list = self.get_ext4_partitions()
        count_dict = self.read_errors(ext4_list)
        exit_num, output_msg = self.generate_dict(count_dict)
        check.add_message(exit_num, output_msg)


def main():
    check = JugglerPassiveCheck("walle_fs_check")
    try:
        log.info("Starting.")
        worker = Checker()
        worker.run(check)
    except JugglerPassiveCheckException as ex:
        check.crit(ex.description)
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == "__main__":
    main()
