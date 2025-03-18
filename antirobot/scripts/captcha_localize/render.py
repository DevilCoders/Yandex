# -*- encoding: utf-8 -*-

import os
import sys
import getopt

from jinja2 import Environment, FileSystemLoader


def render_html(out_file, templ_dir, templ_to_render, base_remplate):
    env = Environment(loader = FileSystemLoader(templ_dir))

    template = env.get_template(templ_to_render)

    context = {'BASE': base_remplate, 'TEMPLATE_NAME': templ_to_render}
    s = template.render(context).encode('utf-8')

    with open(out_file, 'w') as out:
        out.write(s.decode())


if __name__ == "__main__":
    (opts, args) = getopt.getopt(sys.argv[1:], "o:")

    if len(args) < 3:
        print("Usage: %s -o <out_file> <templ-dir> <templ-to-render> <base-template>" % __file__, file=sys.stderr)
        sys.exit(2)

    out_file = dict(opts).get("-o")
    templ_dir = args[0]
    templ_to_render = args[1]
    base_remplate = os.path.basename(args[2])

    render_html(out_file, templ_dir, templ_to_render, base_remplate)
