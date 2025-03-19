"""
Images helpers
"""

import logging
import os.path
from datetime import datetime
from distutils import dir_util

from . import utils


@utils.env_stage('create', fail=True)
def create_staging(conf, **_extra):
    """
    Create 'versioned-images` in staging
    """
    log = logging.getLogger(__name__)
    versioned_images_config = conf.get('versioned_images')
    assert versioned_images_config, 'versioned_images config is empty'

    for image_path, image_ops in versioned_images_config.items():
        from_dir = image_ops['from_dir']
        log.info('copying %s from %s', image_path, from_dir)
        dir_util.copy_tree(
            from_dir,
            image_path,
            update=True,
        )


@utils.env_stage('create', fail=True)
def update_dockerfiles(conf, **_extra):
    """
    Update FROM section in staging docker files.
    """
    log = logging.getLogger(__name__)
    versioned_images_config = conf.get('versioned_images')
    assert versioned_images_config, 'versioned_images config is empty'

    for image_path, image_ops in versioned_images_config.items():
        rewrite_docker_from = image_ops['rewrite_docker_from']
        log.info('updating %s with %s', image_path, rewrite_docker_from)
        _rewrite_first_line(os.path.join(image_path, 'Dockerfile'), image_ops['rewrite_docker_from'])


def _rewrite_first_line(file_path, new_line):
    with open(file_path) as file_descriptor:
        old_lines = file_descriptor.readlines()
    new_lines = [new_line + '\n', '# Overridden by {} at {}\n'.format(__file__, datetime.now())] + old_lines[1:]
    with open(file_path, 'w') as file_descriptor:
        file_descriptor.writelines(new_lines)
