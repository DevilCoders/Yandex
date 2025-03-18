import os
import six

from mkdocs.utils import yaml_load

from mkdocs_yandex import run_context
from mkdocs_yandex.loggers import get_logger

log = get_logger(__name__)


def yaml_read(path):
    docs_dir = run_context.config['docs_dir']

    if os.path.isabs(path):
        yaml_path = path
    else:
        yaml_path = os.path.join(docs_dir, path)

    if not yaml_path.startswith(docs_dir):
        log.error(
            "The included file {0} should not be outside of the 'docs_dir' {1}".format(yaml_path, docs_dir)
        )
        raise SystemExit(1)

    if yaml_path in run_context.tocs:
        log.error(
            "The file {0} is included more than once".format(yaml_path)
        )
        raise SystemExit(1)

    if not os.path.isfile(yaml_path):
        log.error(
            "The file {0} does not exist".format(yaml_path)
        )
        raise SystemExit(1)

    run_context.tocs.append(yaml_path)

    with open(yaml_path, 'r') as f:
        s = f.read()
        jinja2_template = run_context.jinja2_env.from_string(s)
        yaml = yaml_load(jinja2_template.render(**run_context.config['extra']))

    return yaml


def traverse_nav(nav):
    for i, item in enumerate(nav):
        if isinstance(item, six.string_types):
            basename, ext = os.path.splitext(item)
            if ext in ['.yml', '.yaml']:
                yaml = yaml_read(item)
                del nav[i]
                nav[i:i] = yaml
                traverse_nav(nav[i])

        else:
            kv = six.next(six.iteritems(item))
            if isinstance(kv[1], list):
                traverse_nav(kv[1])
            else:
                basename, ext = os.path.splitext(kv[1])
                if ext in ['.yml', '.yaml']:
                    yaml = yaml_read(kv[1])
                    nav[i][kv[0]] = yaml
                    traverse_nav(nav[i][kv[0]])


def preprocess_nav():
    log.info('Processing nav includes')
    run_context.tocs = []
    traverse_nav(run_context.config['nav'])
