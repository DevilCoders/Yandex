#!python
"""
Bot Helper CLI
"""

import json
import os
import logging
import re
from pathlib import Path
from typing import List

import click
import pandas as pd
import pprint
from tabulate import tabulate

from bot_lib.bot_client import BotClient, BOT_ID_INV, BOT_ID_FQDN

from bot_lib.fields import DEFAULT_CALC_FIELDS, C_DISKDRIVES, C_RAM
from bot_lib.fields import DEFAULT_FIELDS

DEBUG = False

CFG_FILE = '~/.config/bot-cli.json'
CFG = {}

logging.basicConfig()
logger = logging.getLogger()
loggingLevel = logging.WARNING if not DEBUG else logging.DEBUG
logger.setLevel(loggingLevel)

OUTPUTS = ["plain", "tabulate", "ttabulate", "json", "csv"]


DEFAULT_CONFIG={
    "output": "plain",
    "verbose": "false",
    "default_servers_fields": DEFAULT_FIELDS,
    "default_server_calculated_fields": DEFAULT_CALC_FIELDS
}


def _load_config(p):
    if not Path(os.path.expanduser(CFG_FILE)).exists():
        return DEFAULT_CONFIG
    with open(os.path.expanduser(CFG_FILE))as fh:
        return json.load(fh)


@click.group()
def cli1():
    pass


cli = click.CommandCollection(sources=[cli1])


@cli1.command("write_config")
def write_config():
    cfg = json.dumps(DEFAULT_CONFIG, indent=4, sort_keys=True)
    with open(os.path.expanduser(CFG_FILE), 'w') as f:
        f.write(cfg)


def get_reduced_dataset(source, fields):
    result = []
    for s in source:
        r = {}
        for f in fields:
            val = s[f]
            if isinstance(val, list):
                r[f] = str(val)
            else:
                r[f] = val
        result.append(r)
    return result


def fmt(servers_r, disks_r, memory_r, total_r, fields, output_fmt, disks, memory, total):
    ds = get_reduced_dataset(servers_r, fields)
    df = pd.DataFrame(ds)
    if output_fmt == "json":
        output = {"servers": ds}
        for s in ds:
            if disks:
                s[C_DISKDRIVES] = disks_r[s["INV"]]
            if memory:
                s[C_RAM] = memory_r[s["INV"]]
        if total:
            output["total"] = total_r
        return json.dumps(output, indent=4, sort_keys=True)
    elif output_fmt == "plain":
        out = ""
        if not disks and not memory:
            out = pprint.pformat(df)
            out = out if not total else out + "\n" + "TOTAL:" + "\n" + pprint.pformat(pd.DataFrame([total_r]))
            return out
        for s in ds:
            if len(out) > 0:
                out += "\n--------------------\n\n"
            out = out + pprint.pformat(pd.DataFrame([s]))
            memory_label = "MEMORY:"
            memory_value = pprint.pformat(pd.DataFrame(memory_r[s["INV"]]))
            out = out if not memory else out + "\n" + memory_label + "\n"+memory_value
            disks_label = "DISKS:"
            disks_value = pprint.pformat(pd.DataFrame(disks_r[s["INV"]]))
            out = out if not disks else out + "\n" + disks_label + "\n" + disks_value
        out = out if not total else out + "\n" + "TOTAL:" + "\n" + pprint.pformat(pd.DataFrame([total_r]))
        return out
    elif output_fmt == "tabulate":
        out = ""
        if not disks and not memory:
            out = tabulate(df, headers='keys', tablefmt='psql')
            out = out if not total else out + "\n" + "TOTAL:" + "\n" + tabulate(pd.DataFrame([total_r]), headers='keys', tablefmt='psql')
            return out
        for s in ds:
            if len(out) > 0:
                out += "\n--------------------\n\n"
            out = out + tabulate(pd.DataFrame([s]), headers='keys', tablefmt='psql')
            memory_label = "MEMORY:"
            memory_value = tabulate(pd.DataFrame(memory_r[s["INV"]]), headers='keys', tablefmt='psql')
            out = out if not memory else out + "\n" + memory_label + "\n"+memory_value
            disks_label = "DISKS:"
            disks_value = tabulate(pd.DataFrame(disks_r[s["INV"]]), headers='keys', tablefmt='psql')
            out = out if not disks else out + "\n" + disks_label + "\n" + disks_value
        out = out if not total else out + "\n" + "TOTAL:" + "\n" + tabulate(pd.DataFrame([total_r]), headers='keys', tablefmt='psql')
        return out
    elif output_fmt == "ttabulate":
        return tabulate(df.T, headers='keys', tablefmt='psql')
    elif output_fmt == "csv":
        return df.to_csv()
    raise Exception("Invalid output format: {}".format(output_fmt))


def detect_id_type(ids: List[str]) -> int:
    if all(re.match("^[0-9]+$", f.strip()) for f in ids):
        return BOT_ID_INV
    if all([re.match("^[-a-zA-Z0-9\.]+$", f.strip()) for f in ids]):
        return BOT_ID_FQDN
    return -1


@click.option('-f', required=False, default=None, help="Comma (,) separated list of fields")
@click.option("--of", required=False, default=None, help="Output formats: {}".format(OUTPUTS))
@click.option("--disks", "-d", required=False, is_flag=True, default=False, help="List of disks")
@click.option("--memory", "-m", required=False, is_flag=True, default=False, help="List of memory (RAM)")
@click.option("--total", "-t", required=False, is_flag=True, default=False, help="Calculate total amount of resources")
@click.argument('ids', nargs=-1)
@cli1.command()
def servers(f: str, of: str, disks: bool, memory: bool, total, ids: List[str]):
    fields = []
    if f is not None and len(f) > 0:
        fields = [x.strip(' ') for x in f.split(",")]
    elif len(fields) == 0:
        fields = CFG["default_servers_fields"]+CFG["default_server_calculated_fields"]

    if of is None or len(of) == 0:
        of = CFG["output"]
    assert of in OUTPUTS

    c = BotClient(logger=logger)
    idType = detect_id_type(ids)
    assert idType == BOT_ID_FQDN or idType == BOT_ID_INV
    servers_r, disks_r, memory_r, total_r = c.servers(idType, ids)
    click.echo(fmt(servers_r, disks_r, memory_r, total_r, fields, of, disks, memory, total))


def __init_cli():
    global CFG
    CFG = _load_config(CFG_FILE)


if __name__ == '__main__':
    __init_cli()
    cli()
