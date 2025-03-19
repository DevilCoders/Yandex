import argparse
import logging
import secrets
import string
import re

import yaml
from cloud.mdb.scripts.pillar_generators.lockbox import LockBox, TextValue


log = logging.getLogger(__name__)


def get_pillar(filename):
    try:
        with open(filename) as fd:
            return yaml.safe_load(fd)
    except yaml.YAMLError as exc:
        log.warning('%s not a yaml file, parse error: %s. jinja not supported', filename, exc)
    return None


def generate_password(length=32):
    alphabet = string.ascii_letters + string.digits
    return ''.join(secrets.choice(alphabet) for i in range(length))


def main():
    parser = argparse.ArgumentParser(
        description="""Generate passwords for pgusers,
         store them in LockBox,
         write secert ids to pillar files,
         grant service account permission to access that passwords"""
    )
    parser.add_argument('pillar_file', nargs='+', help='pillars with pgusers')
    parser.add_argument(
        '--ignore',
        help='ignore given users',
        nargs='*',
    )
    parser.add_argument(
        '--secret-name-prefix',
        default='pguser',
    )
    parser.add_argument(
        '--ycp-profile',
        required=True,
    )
    parser.add_argument(
        '--folder-id',
        required=True,
    )
    parser.add_argument('--service-account-id', help='grant that service account access to all found secrets')
    parser.add_argument(
        '-v',
        '--verbose',
        action='store_true',
        default=False,
        help='print debug logs',
    )

    args = parser.parse_args()
    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

    lockbox = LockBox(args.ycp_profile, args.folder_id)
    ignore = set()
    if args.ignore:
        ignore = set(args.ignore)

    for filename in args.pillar_file:
        pillar = get_pillar(filename)
        if not pillar:
            continue
        do_changes = False

        try:
            users = pillar['data']['config']['pgusers']
        except (KeyError, TypeError) as exc:
            log.warning('filename not a pgusers pillar: %s', exc)
            continue
        for username, opts in users.items():
            if username in ignore:
                continue
            log.info('Examing user: %s', username)
            secret_id = None
            if 'password' not in opts:
                log.info("%s user doesn't have password. Genereate and store it", username)
                secret_key = 'password'
                secert_value = generate_password()
                secret_id = lockbox.create_secret(
                    f'{args.secret_name_prefix}-{username}', TextValue(secret_key, secert_value)
                )
                opts['password'] = '{{ salt.lockbox.get("%s").password }}' % secret_id
                do_changes = True
            else:
                match = re.search(r'salt\.lockbox.get\("(\w+)"\)', opts['password'])
                if match is None:
                    log.warning('unable to extract secret_id from password: %s', opts['password'])
                else:
                    secret_id = match.group(1)
                    log.debug('%s has password in %s secret', username, secret_id)
            if secret_id and args.service_account_id:
                lockbox.add_access_binding(secret_id, args.service_account_id)
        if do_changes:
            log.info('Updating %s', filename)
            with open(filename, 'w') as fd:
                yaml.safe_dump(pillar, fd, indent=4)


if __name__ == '__main__':
    main()
