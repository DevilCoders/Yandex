#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
from yc_monitoring import Status, report_status_and_exit

TPM_FILENAME = '/sys/devices/LNXSYSTM:00/LNXSYBUS:00/MSFT0101:00/description'


def check_tpm_module(tpm_desc_file):

    tpm_exists = False

    if os.path.exists(tpm_desc_file):
        with open(tpm_desc_file) as f:
            if 'TPM 2.0 Device\n' == f.read():
                tpm_exists = True

    return tpm_exists


def main():
    if check_tpm_module(TPM_FILENAME):
        report_status_and_exit(Status.OK, "OK")
    else:
        report_status_and_exit(Status.CRIT, "TPM not found!")


if __name__ == '__main__':
    main()
