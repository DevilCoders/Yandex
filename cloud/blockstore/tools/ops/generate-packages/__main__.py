import argparse
import os

from library.python import resource
from jinja2 import Template


def run():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--cfg-dir', type=str, help='configs dir', required=True)
    parser.add_argument(
        '--pkg-dir', type=str, help='package dir', required=True)
    parser.add_argument(
        '--pkg-name', type=str, help='package name', required=True)

    args = parser.parse_args()

    res_data = resource.find("pkg.json.jinja2").decode('utf8')

    template = Template(res_data)
    pkg_json = template.render(pkg_name=args.pkg_name, cfg_dir=args.cfg_dir)

    if not os.path.exists(args.pkg_dir):
        os.makedirs(args.pkg_dir)

    with open(os.path.join(args.pkg_dir, "pkg.json"), "w+") as f:
        f.write(pkg_json)
        f.write('\n')


if __name__ == '__main__':
    run()
