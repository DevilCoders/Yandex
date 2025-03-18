# Пример запуска:
# TODO

import os
import sys
import argparse

from antirobot.scripts.captcha_localize.render import render_html


if __name__ == "__main__":
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--template-files', action='store', dest='template_files', type=str, nargs='*', default=[])
    parser.add_argument('--out-dir', required=True)
    parser.add_argument('--templ-dir', required=True)
    parser.add_argument('--lang-srcs', action='store', dest='lang_srcs', type=str, nargs='*', default=[])
    args = parser.parse_args()

    if not os.path.exists(args.out_dir):
        os.makedirs(args.out_dir)

    for template_file in args.template_files:
        for lang in args.lang_srcs:
            if "/versions/" in template_file and f"/{lang}/" not in template_file:
                continue

            if "/versions/" in template_file:
                version = template_file.split("/versions/")[1].split("/")[0]
                out_file = os.path.join(args.out_dir, f"{version}-captcha_{template_file.split('/')[-2]}.html.{lang}")
            else:
                out_file = os.path.join(args.out_dir, f"{os.path.basename(template_file)}.{lang}")

            render_html(out_file, args.templ_dir, os.path.join("lang", lang), os.path.relpath(template_file, args.templ_dir))
