import yt.wrapper as yt

from cloud.ai.lib.python.datetime import now
from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data.ops.yt import tables_path, backup_path_tpl


def main():
    yt.config['proxy']['url'] = 'hahn'
    backup_date = Table.get_name(now())
    backup_path = backup_path_tpl.format(backup_date=backup_date)
    if yt.exists(backup_path):
        print(f'Backup on path {backup_path} already exists')
        return
    print(f'Backup tables from {tables_path} to {backup_path}')
    yt.copy(source_path=tables_path, destination_path=backup_path, recursive=True)
