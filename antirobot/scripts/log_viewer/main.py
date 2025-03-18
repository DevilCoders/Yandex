import pyuwsgi
from argparse import ArgumentParser
from antirobot.scripts.log_viewer.app import app as application  # noqa


def main():
    parser = ArgumentParser()
    parser.add_argument('--ini', dest='ini', required=True, help='uwsgi ini configuration file')
    args = parser.parse_args()

    args_pass = [
        '--callable', 'application',
        '--ini', args.ini,
        '-w', 'main',
        '--master',
    ]
    pyuwsgi.run(args_pass)

if __name__ == "__main__":
    main()
