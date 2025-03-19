import argparse
import cloud.iam.ydb_tools.backup.lib.backup as backup


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-yt-dir', '--yt-directory', help='destination YT directory, e.g. //home/myfolder/preprod', required=True)
    parser.add_argument('-fs-dir', '--fs-directory', help='file system YDB backup directory - all tables are relative to this folder', required=True)
    parser.add_argument('-t', '--table', help='table to be exported', action='append', type=str,
                        default=[], nargs=3, metavar=['FS_TABLE', 'YT_TABLE' 'COLUMNS'])
    parser.add_argument('-ttl', '--ttl-days', help='export TTL in days, default is 14 days', default=14)

    args = parser.parse_args()
    backup.backup(args.yt_directory, args.fs_directory, args.table, args.ttl_days)


if __name__ == '__main__':
    main()
