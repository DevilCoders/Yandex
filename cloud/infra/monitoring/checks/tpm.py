#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
from typing import List
from yc_monitoring import JugglerPassiveCheck

TPM_FILENAME = '/sys/devices/LNXSYSTM:00/LNXSYBUS:00/MSFT0101:00/description'
ALLOWED_VERSIONS = ['TPM 2.0 Device']


def check_tpm_module(check: JugglerPassiveCheck, tpm_desc_file: str, allowed_tpm_versions: List[str]):
    if os.path.exists(tpm_desc_file):
        with open(tpm_desc_file) as f:
            tpm_version = f.read().rstrip()

        if tpm_version not in allowed_tpm_versions:
            check.crit("Incorrect TPM module version: {}".format(tpm_version))

    else:
        check.crit("TPM module not found!")


def main():
    check = JugglerPassiveCheck("tpm")
    try:
        check_tpm_module(check, TPM_FILENAME, ALLOWED_VERSIONS)
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == '__main__':
    main()
