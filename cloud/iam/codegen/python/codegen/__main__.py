# -*- coding: utf-8 -*-
import os
import jinja2
import cloud.iam.codegen.python.codegen.libcodegen as libcodegen


# import libcodegen


def generate(args):
    template = args.template
    model = args.model
    output = args.output
    environment = libcodegen.load_environment(model)
    jinja_env = jinja2.Environment()
    jinja_env.loader = jinja2.FileSystemLoader(searchpath=os.path.dirname(os.path.abspath(template)), followlinks=True)
    jtemplate = jinja_env.get_template(os.path.basename(template))
    result = jtemplate.render(env=environment)
    if not output:
        if template.endswith('.tmpl'):
            output = template[:-len('.tmpl')]
        else:
            output = template + '.gen'
    if output == '-':
        print(result)
    else:
        with open(output, 'w') as of:
            of.write(result)


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('-t', '--template', help='jinja template file')
    parser.add_argument("-m", "--model", help="path to fixture_permissions.yaml file")
    parser.add_argument("-o", "--output", dest="output", help="output file")
    args = parser.parse_args()
    generate(args)


if __name__ == '__main__':
    main()
