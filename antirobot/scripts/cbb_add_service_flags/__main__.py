import argparse
import json
import logging
import sys
from psycopg2 import connect, ProgrammingError

logger = logging.getLogger(__name__)
handler = logging.StreamHandler(stream=sys.stderr)
logger.addHandler(handler)
logger.setLevel(logging.DEBUG)


def parser_args():
    parser = argparse.ArgumentParser(description='Create groups for service')
    parser.add_argument('-f', '--service-config-file', help='service config file', required=True)
    parser.add_argument('-s', '--service-name', help='service name', required=True)
    parser.add_argument('-S', '--service-human-name', help='service human name, used in wiki', required=True)
    parser.add_argument('-d', '--db-host', help='database host', required=True)
    parser.add_argument('-D', '--db-name', help='database name', required=True)
    parser.add_argument('-u', '--db-username', help='database username', required=True)
    parser.add_argument('-p', '--db-password', help='database password', required=True)
    opts = parser.parse_args()
    return opts


def fetch_query(opts, query):
    host, port = opts.db_host.split(':')
    logger.info('Query:')
    logger.info(query)

    cur = None
    conn = None
    try:
        conn = connect(
            host=host,
            port=port,
            dbname=opts.db_name,
            user=opts.db_username,
            password=opts.db_password
            )

        cur = conn.cursor()
        cur.execute(query)
        conn.commit()
        try:
            return list(cur.fetchall())
        except ProgrammingError:
            return None
    except Exception:
        conn.rollback()
        logger.error('Cannot execute query')
        raise
    finally:
        if cur is not None:
            cur.close()
        if conn is not None:
            conn.close()


def main():
    opts = parser_args()

    service = opts.service_name
    groups = [
        (f'{service}_captcha_ip', 'cbb_farmable_ban_flag'),
        (f'{service}_403_ip', 'cbb_ip_flag'),
        (f'{service}_captcha_re', 'cbb_captcha_re_flag'),
        (f'{service}_403_re', 'cbb_re_flag'),
        (f'{service}_mark_re', 'cbb_re_user_mark_flag'),
    ]

    with open(opts.service_config_file) as f:
        service_config = json.load(f)

    wiki_data = {}

    for service in service_config:
        if service.get('service') == opts.service_name:
            for group_name, config_field in groups:
                result = fetch_query(opts, f"SELECT id FROM cbb.cbb_groups WHERE name = '{group_name}';")
                logger.info('Result:')
                logger.info(result)
                group_id = int(result[-1][0])
                logger.info(f'Found: {group_name}, {group_id}')

                wiki_data[config_field] = group_id

                cbb_flags = service.get(config_field)
                if isinstance(cbb_flags, int):
                    cbb_flags = [cbb_flags]
                if group_id not in cbb_flags:
                    cbb_flags.append(group_id)
                service[config_field] = cbb_flags

    with open(opts.service_config_file, 'w') as f:
        print(json.dumps(service_config, sort_keys=True, indent=4), file=f)

    print('commmit new version of service_config')
    print("Pleease add to wiki: https://wiki.yandex-team.ru/jandekspoisk/sepe/antirobotonline/servicecbb/.edit?section=3&goanchor=h-3&originpath=/jandekspoisk/sepe/antirobotonline/servicecbb")
    print('')
    wiki_result = []
    wiki_result.append(f'((https://cbb.n.yandex-team.ru/tag/cbb{opts.service_name} {opts.service_human_name}))')
    for field in ['cbb_farmable_ban_flag', 'cbb_captcha_re_flag', 'cbb_ip_flag', 'cbb_re_flag', 'cbb_re_user_mark_flag']:
        group_id = wiki_data[field]
        wiki_result.append(f'((https://cbb.n.yandex-team.ru/groups/{group_id} {group_id}))')
    wiki = '| ' + ' | '.join(wiki_result) + ' |'
    print(wiki)


if __name__ == '__main__':
    main()
