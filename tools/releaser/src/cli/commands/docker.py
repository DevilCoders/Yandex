# coding: utf-8
import os
import json

import click

from gitchronicler import chronicler
from gitchronicler.ctl import get_vcs_ctl

from tools.releaser.src.conf import cfg
from tools.releaser.src.cli import options, utils


@click.command(
    help='build image',
)
@options.image_option
@options.buildfile_option
@options.sandbox_option
@options.dockerfile_option
@options.version_option
@options.version_build_arg_option
@options.dry_run_option
@options.pull_base_image_option
def build(image, buildfile, sandbox, dockerfile, version, version_build_arg, dry_run, pull_base_image):
    version = version or chronicler.get_current_version()

    registry, repository = image.split('/')[:2]

    if sandbox:
        context = {
            'checkout_arcadia_from_url': 'arcadia-arc:/#trunk',
            'package_type': 'docker',
            'docker_registry': registry,
            'docker_image_repository': repository,
            'docker_push_image': True,
            'custom_version': version,
        }

        if buildfile is not None:
            output, err = get_vcs_ctl().get_output('root')
            if err != 0:
                utils.panic('buildfile should be inside arc repository')
            root = output[0].strip()
            package = buildfile[len(root) + 1:]
            context['packages'] = package

        owner = cfg.SANDBOX_CONFIG.pop('owner', os.getenv('LOGNAME'))

        context.update(cfg.SANDBOX_CONFIG)

        utils.run_with_output(
            'ya', 'tool', 'sandboxctl', 'create',
            '--name', 'YA_PACKAGE',
            '--ctx-jsonstr', json.dumps(context),
            '--owner', owner,
            '--wait',
            _dry_run=dry_run,
        )
        return

    if buildfile:
        utils.run_with_output(
            'ya', 'package', buildfile, '--docker',
            '--docker-registry', registry,
            '--docker-repository', repository,
            '--custom-version', version,
            _dry_run=dry_run,
        )
        return

    args = ['docker', 'build', '-f', dockerfile, '-t', f'{image}:{version}']

    if pull_base_image:
        args.append('--pull')

    if version_build_arg:
        args += ['--build-arg', 'APP_VERSION=%s' % version]
    args += ['.']

    utils.run_with_output(*args, _dry_run=dry_run)


@click.command(help='push image')
@options.sandbox_option
@options.image_option
@options.version_option
@options.dry_run_option
def push(sandbox, image, version, dry_run):
    version = version or chronicler.get_current_version()

    if sandbox:
        return

    utils.run_with_output('docker', 'push', f'{image}:{version}', _dry_run=dry_run)
