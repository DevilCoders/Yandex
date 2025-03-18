import argparse
import logging

import library.python.oauth


def main():
    args_parser = argparse.ArgumentParser(description='OAuth token fetcher')
    args_parser.add_argument('id', help='Client ID')
    args_parser.add_argument('secret', help='Client secret')
    args_parser.add_argument('--login', help='User login')
    args_parser.add_argument('--use-password', action='store_true', help='Use password instead of SSH keys')
    args_parser.add_argument('--verbose', action='store_true', help='Verbose logging')
    args = args_parser.parse_args()

    if args.verbose:
        logger = logging.getLogger()
        logger.setLevel(logging.DEBUG)
        log_handler = logging.StreamHandler()
        logger.addHandler(log_handler)

    get_token = library.python.oauth.get_token_by_password if args.use_password else library.python.oauth.get_token
    print get_token(args.id, args.secret, args.login)


if __name__ == '__main__':
    main()
