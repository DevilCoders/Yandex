#!/usr/bin/env python3
"""
Simple script to adjust database.slice limits based on porto-container limits
"""

import json
import os
import subprocess

import porto


def get_config():
    """
    Get configuration from /etc/database-slice-adjuster.conf
    """
    with open('/etc/database-slice-adjuster.conf') as conf:
        return json.load(conf)


def get_porto_limits():
    """
    Get current container porto limits
    """
    res = {}
    conn = porto.Connection()
    conn.connect()
    container = conn.Find('self')
    res['cpu'] = int(container.GetProperty('cpu_limit').replace('c', ''))
    res['memory'] = int(container.GetProperty('memory_limit'))
    return res


def adjust_limits(current_limits, config):
    """
    Render new slice config and do atomic update/systemd reload on changes
    """
    with open(config['path']) as slice_path_file:
        current_buf = slice_path_file.read()

    proposed_buf = '[Slice]\n'
    if config['cpu']['enabled']:
        proposed_buf += 'CPUAccounting=yes\nCPUWeight=1\n'
        proposed_buf += f'CPUQuota={int(current_limits["cpu"] * 100 - config["cpu"]["max_reserved"])}%\n'
    if config['memory']['enabled']:
        proposed_buf += 'MemoryAccounting=yes\n'
        reserved = int(min((current_limits['memory'] * config['memory']['multiplier_reserved_memory'],
                            config['memory']['max_reserved'])))
        advised = int(min((current_limits['memory'] * config['memory']['multiplier_advised_memory'],
                           config['memory']['max_advised'])))
        proposed_buf += f'MemoryHigh={current_limits["memory"] - advised}\n'
        proposed_buf += f'MemoryMax={current_limits["memory"] - reserved}\n\n'

    if current_buf != proposed_buf:
        tmp_path = f'{config["path"]}.tmp'
        with open(tmp_path, 'w') as out_file:
            out_file.write(proposed_buf)
        os.rename(tmp_path, config['path'])
        subprocess.check_call(['/bin/systemctl', 'daemon-reload'])


def main():
    """
    Console entry-point
    """
    config = get_config()
    current_limits = get_porto_limits()
    if config['cpu']['limit'] <= current_limits['cpu'] or config['memory']['limit'] <= current_limits['memory']:
        adjust_limits(current_limits, config)


if __name__ == '__main__':
    main()
