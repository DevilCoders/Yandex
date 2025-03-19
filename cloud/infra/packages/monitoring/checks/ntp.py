#!/usr/bin/env python3
"""Network time protocol time difference"""

import re

from ycinfra import Popen
from yc_monitoring import JugglerPassiveCheck


class NtpCheck(JugglerPassiveCheck):
    YA_NTP_SERVERS_STATUS = {}
    RE_PATTERN = re.compile(r'(?P<server_ip>(\d{1,3}\.){3}\d{1,3})$')
    NTP_STRATUM = 4
    NTP_DELAY = 40  # in milliseconds
    NTP_OFFSET = 80  # in milliseconds
    NTP_JITTER = 40  # in milliseconds

    def __init__(self):
        super(NtpCheck, self).__init__("ntp")

    @staticmethod
    def _read_ntp_stats(check):
        command = "ntpq -p"
        returncode, stdout, stderr = Popen().exec_command(command)
        if returncode != 0:
            check.crit("Command: {}. ret_code: {}. STDERR: {}".format(command, returncode, stderr))
        ntp_info = stdout.splitlines()
        return ntp_info

    def _get_ntp_info(self):

        for line in self._read_ntp_stats(self):
            fields_line = line.split()
            if len(fields_line) < 10:
                continue
            match = self.RE_PATTERN.match(fields_line[1])
            if not match:
                continue
            ntp_server = fields_line[0][1:]
            # stratum: NTP server level
            # delay: network round trip time (in milliseconds) to ntp server
            # offset: difference between local clock and remote clock (in milliseconds)
            # jitter: difference of successive time values from server
            self.YA_NTP_SERVERS_STATUS[ntp_server] = {
                "stratum": int(fields_line[2]),
                "delay": float(fields_line[7]),
                "offset": float(fields_line[8]),
                "jitter": float(fields_line[9]),
            }
        return self.YA_NTP_SERVERS_STATUS

    def check_ntp_params(self):
        found_ntp_servers = 0
        high_difference_errors = []

        for ntp_server, params in self._get_ntp_info().items():
            found_ntp_servers += 1
            if params["stratum"] > self.NTP_STRATUM:
                self.warn("Warning: NTP server {} has too low ntp stratum: {}".format(ntp_server, params["stratum"]))
            if params["delay"] > self.NTP_DELAY:
                self.warn("Warning: Too high network round trip time: {} to ntp server {}".format(
                    params["delay"], ntp_server))
            if params["offset"] > self.NTP_OFFSET:
                high_difference_errors.append(
                    "Error: Too high difference: {} between local clock and ntp server {}".format(
                        params["offset"], ntp_server))
            if params["jitter"] > self.NTP_JITTER:
                self.warn("Warning: Too high difference: {} of successive time values from server {}".format(
                    params["jitter"], ntp_server))

        if found_ntp_servers == 0:
            self.crit("Lost all NTP servers")
            return

        if len(high_difference_errors) == 1:
            if found_ntp_servers == 1:
                self.crit(high_difference_errors[0])
            else:
                self.warn(high_difference_errors[0])
        else:
            for message in high_difference_errors:
                self.crit(message)


def main():
    check = NtpCheck()
    try:
        check.check_ntp_params()
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == '__main__':
    main()
