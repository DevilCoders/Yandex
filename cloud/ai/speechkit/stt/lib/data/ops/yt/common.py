tables_path = '//home/mlcloud/speechkit/stt'
backup_path_tpl = '//home/mlcloud/backup/{backup_date}/speechkit/stt'


def get_tables_dir(dir: str) -> str:
    return f'{tables_path}/{dir}'
