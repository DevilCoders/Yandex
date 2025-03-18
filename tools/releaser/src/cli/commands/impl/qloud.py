import os
import sys
import random
import socket

import click
import hjson

from gitchronicler import chronicler

from tools.releaser.src.cli import utils
from tools.releaser.src.conf import parse_config
from tools.releaser.src.lib import qloud, docker


def status(qloudinst, project, applications, environment, dry_run):
    utils.maybe_download_certificate(dry_run=dry_run)

    oauth_token = utils.get_oauth_token_or_panic()
    qloud_client = qloud.QloudClient(qloudinst, oauth_token)

    for application in applications:
        environment_object = qloud.QloudObject(
            project=project,
            application=application,
            environment=environment,
            component=None,
        )

        if dry_run:
            click.echo('[DRY RUN] displaying status for {}'.format(environment_object))
            continue

        status_data = qloud_client.get_environment_status(environment_object)

        # Сортируем версии инстансов от новых к старым, чтобы позже оставить
        # только самые свежие версии инстансов.
        status_data.sort(reverse=True, key=lambda x: x['instanceVersion'])

        state = {}

        for instance in status_data:
            name = instance['instanceName']
            if name not in state:
                state[name] = instance

        click.echo('{}:'.format(environment_object))

        if state:
            for component, info in sorted(state.items(), key=lambda x: x[0]):
                click.echo(
                    '  {instanceName} [{dc}]: {instanceCurrentState}'
                    .format(name=component, **info)
                )
        else:
            click.echo('  [no instances]')


def process_component_options(components):
    """
    Из словаря заданного в настройке компонент делает qloud-настройки instanceOptions.

    >>> process_component_options(['c1', 'c2']) == {'c1': {}, 'c2': {}}
    True
    >>> components = dict(backend=dict(IVA=1, MYT=1), front=dict(MOSCOW=1))
    >>> components_cfg = process_component_options(components)
    >>> set(components.keys()) == set(components_cfg.keys())
    True
    >>> components_cfg = process_component_options(dict(api=dict(IVA=1)))
    >>> components_cfg == dict(api=dict(instanceGroups=[dict(location='IVA', units=1, backup=False, weight=1)]))
    True
    """
    if isinstance(components, (list, tuple)):
        return {component: {} for component in components}

    result = {}
    for component, opts in components.items():
        if not opts:
            result[component] = opts
            continue
        instance_groups = list(
            dict(
                location=dc,
                units=int(num_instances),
                backup=False,
                weight=1,
            ) for dc, num_instances in opts.items())
        result[component] = dict(instanceGroups=instance_groups)
    return result


def _get_prev_version(prev_env_images):
    try:
        prev_versions = set(
            utils.get_version_from_image_url(image)
            for image in prev_env_images)
    except (TypeError, ValueError) as exc:
        click.echo(
            "WARNING: Failed to find previous version from images {!r}: {!r}".format(prev_env_images, exc),
            err=True)
        return None

    if len(prev_versions) == 1:
        return list(prev_versions)[0]

    click.echo(
        "WARNING: Multiple previous versions found: {!r}".format(prev_versions),
        err=True)
    try:
        return min(prev_versions, key=utils.version_sort_key)
    except TypeError:
        click.echo(
            "WARNING: Failed to properly sort such versions.",
            err=True)
        return min(prev_versions)


def deploy(image, version, qloudinst, project, applications, environment, components, deploy_comment_format,
           target_state=None, dump=None, dry_run=None, var=None, from_version=None):

    components = process_component_options(components)

    oauth_token = utils.get_oauth_token_or_panic()
    docker_info_client = docker.DockerInfoClient(oauth_token)
    qloud_client = qloud.QloudClient(qloudinst, oauth_token)

    image_hash = docker_info_client.get_image_hash(image_url_without_tag=image, image_tag=version)

    for application in applications:
        environment_object = qloud.QloudObject(
            project=project,
            application=application,
            environment=environment,
            component=None,
        )

        # Мы могли бы вместо вызова upload всего окружения вызывать deploy для каждого компонента:
        # (https://docs.qloud.yandex-team.ru/doc/api/components_and_services#deploycomponent)
        # Однако в этом случае не сработает ситуация, когда у двух и более компонентов до деплоя
        # была одинаковая версия TAG, и после деплоя также осталась версия TAG, но уже с другим хешем
        # (в этом случае Qloud вернет ошибку при попытке деплоя).

        if dump:
            environment_dump = parse_config(dump)
            environment_dump['objectId'] = str(environment_object)
        else:
            environment_dump = qloud_client.get_environment_dump(environment_object)

        if var:
            def parse_env_var(text):
                if '=' in text:
                    return text.split('=', 1)
                # else:
                raise RuntimeError(
                    'Неверный формат для переменной окружения: {}\n'
                    'Должно быть что-то вроде: BLAH=minor'.format(text)
                )
            env_vars = map(parse_env_var, var)
            for key, value in env_vars:
                environment_dump['userEnvironmentMap'][key] = value

        prev_env_images = []
        for component in environment_dump['components']:
            component_name = component['componentName']

            if component_name in components:
                opts = components[component_name]
                prev_env_images.append(component['properties']['repository'])
                component['properties']['repository'] = f'{image}:{version}'
                component['properties']['hash'] = image_hash

                if opts:  # see `process_component_options`
                    component.update(opts)

        if from_version is None:
            from_version = _get_prev_version(prev_env_images)

        deploy_comment = utils.get_deploy_comment(
            deploy_comment_format,
            changelog_records=chronicler.get_changelog_records(),
            from_version=from_version,
            new_version=version)

        if not dry_run:
            qloud_client.upload_environment(
                environment_dump=environment_dump, comment=deploy_comment,
                target_state=target_state)
        else:
            click.echo('\n'.join([
                '[DRY RUN] deploying environment dump:',
                hjson.dumps(environment_dump, indent=4),
                'Version: {} -> {}'.format(utils.reprish(from_version), utils.reprish(version)),
                'Comment:',
                hjson.dumps(deploy_comment).strip(),
            ]))
            click.echo('[DRY RUN] ', nl=False)

        components_str = '(%s)' % ','.join(components)
        environment_url = utils.get_full_environment_url(qloudinst, environment_object)

        click.echo(click.style('Deploy %s.%s in progress!' % (str(environment_object), components_str), fg='green'))
        click.echo(click.style(environment_url, underline=True))


def add_domain(qloudinst, project, application, environment, domain, domain_type, dry_run):
    oauth_token = utils.get_oauth_token_or_panic()
    qloud_client = qloud.QloudClient(qloudinst, oauth_token)

    environment_object = qloud.QloudObject(
        project=project,
        application=application,
        environment=environment,
        component=None,
    )

    domains = qloud_client.get_domains(environment_object)
    domains_data = [(d['domainName'], d['type']) for d in domains]

    if (domain, domain_type) in domains_data:
        click.echo(
            ('domain {domain!r} (type={domain_type!r}) already exists'
             ' in environment {environment_object}').format(
                 domain=domain, domain_type=domain_type, environment_object=environment_object),
            err=True)
        return

    if not dry_run:
        qloud_client.add_domain(environment_object, domain, domain_type)
    else:
        click.echo(
            '[DRY RUN] Adding domain "%s" (type="%s") to environment "%s"' % (
                domain,
                domain_type,
                str(environment_object))
            )


def add_deploy_hook(qloudinst, project, application, environment, deploy_hook, dry_run):
    utils.maybe_download_certificate(dry_run=dry_run)
    if deploy_hook is None:
        click.echo('Skip. You should set deploy-hook argument', err=True)
        return

    oauth_token = utils.get_oauth_token_or_panic()
    qloud_client = qloud.QloudClient(qloudinst, oauth_token)

    environment_object = qloud.QloudObject(
        project=project,
        application=application,
        environment=environment,
        component=None,
    )

    if not dry_run:
        qloud_client.add_deploy_hook(environment_object, deploy_hook)
    else:
        click.echo(
            '[DRY RUN] Adding deploy hook "%s" to environment "%s"' % (
                deploy_hook,
                str(environment_object))
            )


def env_delete(qloudinst, project, application, environment, non_interactive):
    oauth_token = utils.get_oauth_token_or_panic()
    qloud_client = qloud.QloudClient(qloudinst, oauth_token)

    if '*' not in environment:
        environments = [qloud.QloudObject(
            project=project,
            application=application,
            environment=environment,
            component=None,
        )]
    else:
        environments = qloud.get_environments_by_wildcard(
            project_details=qloud_client.get_project(qloud.QloudObject(
                project=project,
                application=None,
                environment=None,
                component=None,
            )),
            application=application,
            environment_wildcard=environment,
        )
        if not environments:
            utils.panic('No environments found')

        if not non_interactive:
            click.confirm('You are about to delete environments:\n%s'
                          % '\n'.join(str(e) for e in environments))

    for environment_object in environments:
        qloud_client.delete_environment(environment_object)

        click.echo('Deleting %s' % str(environment_object))


def env_dump(qloudinst, project, application, environment):
    oauth_token = utils.get_oauth_token_or_panic()
    qloud_client = qloud.QloudClient(qloudinst, oauth_token)
    environment_object = qloud.QloudObject(project=project, application=application, environment=environment, component=None)
    dump = qloud_client.get_environment_dump(environment_object)
    return hjson.dumps(dump)


def _list_hosts(qloudinst, project, application, environment, components=None, require=False, dry_run=False):
    ''' Shared helper for `hosts` and `ssh` and `pssh` '''

    utils.maybe_download_certificate(dry_run=dry_run)

    oauth_token = utils.get_oauth_token_or_panic()
    qloud_client = qloud.QloudClient(qloudinst, oauth_token)

    environment_object = qloud.QloudObject(
        project=project,
        application=application,
        environment=environment,
        component=None,
    )

    environment_stable = qloud_client.get_stable_environment(environment_object)
    components_data = environment_stable['components']

    if not components:
        components = sorted(components_data)
    else:
        for component in components:
            if component not in components_data:
                environment_url = utils.get_full_environment_url(qloudinst, environment_object)
                utils.panic('No component "%s" in %s' % (component, environment_url))

    running_instances_data = list(
        item
        for component in components
        for item in components_data[component]['runningInstances'])

    if require and not running_instances_data:
        utils.panic('No running instances for component')

    return list(data['host'] for data in running_instances_data)


def hosts(qloudinst, project, application, environment, components):
    hostlist = _list_hosts(
        qloudinst=qloudinst, project=project, application=application,
        environment=environment, components=components,
        require=True, dry_run=False)
    # Not using `click.echo` because this is an output for use in scripts, not
    # for reading by the user.
    sys.stdout.write(''.join('{}\n'.format(host) for host in hostlist))


def ssh(qloudinst, project, application, environment, components, shellwrap, dry_run, cmd_args, command):
    hostlist = _list_hosts(
        qloudinst=qloudinst, project=project, application=application,
        environment=environment, components=components,
        require=True, dry_run=dry_run)
    return utils.ssh_to_random_host(
        host_urls=hostlist, shellwrap=shellwrap, dry_run=dry_run,
        cmd_args=cmd_args, command=command)


def pssh(qloudinst, project, application, environment, components, shellwrap, dry_run, pssh_cmd, cmd_args):
    """

    Пример use-case для shellwrap:

        $ releaser pssh -s -- -v -P -- python -c 'import sys; print(sys.path)'

    Без этого флага квотирование приходится делать руками:

       $ releaser pssh -- -v -P -- "python -c 'import sys; print(sys.path)'"

    что может быть крайне неудобно когда кавычек становится больше разных.
    """
    hostlist = _list_hosts(
        qloudinst=qloudinst, project=project, application=application,
        environment=environment, components=components,
        require=True, dry_run=dry_run)

    click.echo('Running instances:\n{}\n'.format('\n'.join(hostlist)), err=True)

    cmd_base = [pssh_cmd]
    for host in hostlist:
        cmd_base += ['-H', host]
    cmd_string = utils.sh_join(cmd_args, pre_args=cmd_base, shellwrap_ext=shellwrap)

    if dry_run:
        utils.run_with_output(
            cmd_string, _dry_run=dry_run)
    else:
        os.system(cmd_string)
