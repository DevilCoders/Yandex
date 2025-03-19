"""
Renders configs using all available context: config, docker-compose.yaml, state
"""
import logging
import os
import shutil

from jinja2 import Environment, FileSystemLoader, StrictUndefined

from . import crypto, utils

EXCLUDE_EXT = ['.swp']


@utils.env_stage('create', fail=True)
def render_configs(state, conf):
    """
    Render each template in the subtree.
    Templates are evaluated in source dir ('images' by default),
    result is outputted into staging dir ('staging/images' by default).

    Regular non-template files are written without modification.
    """
    log = logging.getLogger(__name__)
    template_src = 'images'
    template_dst = conf.get('staging_dir', 'staging')
    context = {
        'conf': conf,
        'ssl': state['ssl'],
        'salt_pki': state['salt_pki'],
        'ssh_pki': state['ssh_pki'],
    }
    # Render configs only for projects that are
    # present in config file.
    for project, project_conf in conf.get('projects').items():
        project_template_src = os.path.join(template_src, project_conf.get('templates-inherits-from', project))
        log.debug('rendering configs for %s project [src %s]', project, project_template_src)
        for root, _, files in os.walk(project_template_src):
            for basename in files:
                if any([basename.endswith(ext) for ext in EXCLUDE_EXT]):
                    continue
                # Replace 'project' in path
                # ( project inherits templates
                #   postgresql-10_1c inherits postgresql templates )
                dst_root = root.split(os.path.sep)
                dst_root[1] = project
                # Prepend 'staging' dir to all paths
                dst_dir = os.path.join(template_dst, *dst_root)
                log.debug('rending file %s. src_dir: %s dst_dir: %s', basename, root, dst_dir)
                render_templates_dir(
                    context=context,
                    basename=basename,
                    src_dir=root,
                    # Prepend 'staging' dir to all paths
                    dst_dir=dst_dir,
                )


def getenv(loader=None):
    """
    Create Jinja2 env object
    """
    env = Environment(
        autoescape=False,
        trim_blocks=False,
        undefined=StrictUndefined,
        keep_trailing_newline=True,
        loader=loader,
    )
    env.filters['argon2'] = crypto.argon2
    return env


def render_templates_dir(context, basename, src_dir, dst_dir):
    """
    Renders the actual template.
    """
    # Full filenames
    src_path = os.path.join(src_dir, basename)
    dst_path = os.path.join(dst_dir, basename)

    # Evaluate source dir for templates
    loader = FileSystemLoader(src_dir)
    # Init Environment
    env = getenv(loader)
    try:
        try:
            with open(dst_path, 'w') as template_file:
                template_file.write(env.get_template(basename).render(context))
        except UnicodeDecodeError:
            shutil.copyfile(src_path, dst_path)
    except Exception as exc:
        raise RuntimeError("'{exc_type}' while rendering '{name}': {exc}".format(
            exc_type=exc.__class__.__name__,
            name=src_path,
            exc=exc,
        ))
