"""
Custom outputter for bootstrap. Writes in separate sources:
   - json (specified by environment variable)
   - as default pretty print

This outputter will be used in salt-call via <--out bootstrap> params"""

import json
import os
import re

import salt.loader

YC_BS_SALT_FILE = "YC_BS_SALT_FILE"

__virtualname__ = 'bootstrap'


def __virtual__():
    return __virtualname__


def output(data, **kwargs):  # pylint: disable=unused-argument
    yc_bs_salt_file = os.environ.get(YC_BS_SALT_FILE)
    if not yc_bs_salt_file:
        fmtmsg = "You must specifi environment variable <{}> when selecting outputter <{}>"
        raise Exception(fmtmsg.format(YC_BS_SALT_FILE, __virtualname__))

    # write as json
    with open(yc_bs_salt_file, "w") as f:
        f.write(json.dumps(data, default=repr, indent=4))
    try:
        saved_output = __opts__["output"]  # pylint: disable=E0602
        __opts__["output"] = "highstate"  # pylint: disable=E0602

        text_output = salt.loader.outputters(__opts__)["highstate"](data, **kwargs)  # pylint: disable=E0602,E1101

        # CLOUD-12465: put salt command output to separate place
        yc_bs_salt_file_text = re.sub(".json$", ".txt", yc_bs_salt_file)
        if yc_bs_salt_file_text == yc_bs_salt_file:
            yc_bs_salt_file_text = "{}.txt".format(yc_bs_salt_file)
        with open(yc_bs_salt_file_text, "w") as f:
            f.write(text_output)

        return text_output
    finally:
        __opts__["output"] = saved_output  # pylint: disable=E0602
