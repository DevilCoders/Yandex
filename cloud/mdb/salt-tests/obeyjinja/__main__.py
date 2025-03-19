#!/usr/bin/env python
import argparse
import configparser
import sys

import jinja2
import jinja2.nodes
import jinja2.exceptions

argparser = argparse.ArgumentParser(description='Process some integers.')
argparser.add_argument('--config', type=str, default='.obeyjinjarc', help='config file')
argparser.add_argument('file', nargs='*', default=[], help='list of files to lint')
args = argparser.parse_args()

config = configparser.ConfigParser()
config['DEFAULT']['jinja_extensions'] = 'jinja2.ext.do'
config['DEFAULT']['checks'] = ''
config['DEFAULT']['max_depth'] = '3'
config['DEFAULT']['max_code_percent'] = '100'
config.read(args.config)
config = config['DEFAULT']

jinja_extensions = [e.strip() for e in config['jinja_extensions'].split(',')]
env = jinja2.Environment(extensions=jinja_extensions)


def check_max_depth(_, tpl, config):
    max_depth = int(config['max_depth'])
    issues = []

    def check_node(node, depth):
        if depth > max_depth:
            issues.append('line {}: max depth ({}) exceeded'.format(node.lineno, max_depth))
            return
        if isinstance(node, jinja2.nodes.Stmt):
            depth += 1
        for child in node.iter_child_nodes():
            check_node(child, depth)

    check_node(tpl, 0)
    return issues


def check_code_percent(source, tpl, config):
    max_percent = max(0, min(int(config['max_code_percent']), 100)) / 100
    text_len = [0]

    def check_node(node):
        if isinstance(node, jinja2.nodes.TemplateData):
            text_len[0] += len(node.data)
        for child in node.iter_child_nodes():
            check_node(child)

    check_node(tpl)
    if (len(source) - text_len[0]) / (len(source) or 1) > max_percent:
        return ['template tags take more than {} percent of source'.format(max_percent)]
    return []


checks_to_code = {
    'max_depth': check_max_depth,
    'code_percent': check_code_percent,
}


def lint_file(file):
    source = ''
    with open(file) as fh:
        source = fh.read()
    try:
        tpl = env.parse(source)
    except jinja2.exceptions.TemplateSyntaxError as err:
        return ['line {}: {}'.format(err.lineno, str(err))]
    checks = [c.strip() for c in config['checks'].split(',') if c.strip()]
    if not checks:
        checks = checks_to_code.keys()
    issues = []
    for check in checks:
        if check not in checks_to_code:
            raise Exception("unknown check: " + check)
        issues += checks_to_code[check](source, tpl, config)
    return issues


has_issues = False

for file in args.file:
    issues = lint_file(file)
    if issues:
        has_issues = True
        for issue in issues:
            print(file + ': ' + issue, file=sys.stderr)

sys.exit(1 if has_issues else 0)
