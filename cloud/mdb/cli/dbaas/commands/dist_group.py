from click import argument, group, pass_context, option, Choice, ClickException

from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.dbaas.internal.dist import dist_find, dist_move, dist_copy


@group('dist')
def dist_group():
    """Dist management commands."""
    pass


@dist_group.command('find')
@argument('name')
@option('-r', '--repo', 'repository')
@option('-e', '--env')
@option('-v', '--version')
@option('--format', type=Choice(['raw', 'json', 'yaml']), default='raw')
@pass_context
def find_command(ctx, name, repository, env, version, format):
    """Find packages by name and optional filters."""
    raw_output = format == 'raw'
    response = dist_find(name, repository, env, version, raw_output=raw_output)

    if raw_output:
        print(response)
    else:
        print_response(ctx, response, format=format)


@dist_group.command('move')
@argument('name')
@argument('version')
@option('-r', '--repo', 'repository')
@option('-e', '--env', 'target_environment', default='stable')
def move_command(name, version, repository, target_environment):
    """Move package to the specified environment."""
    packages = dist_find(name, repository, version=version)
    if not packages:
        raise ClickException(f'No {name} packages with version {version} were found')

    for repo, package in {p['repository']: p for p in packages}.items():
        print(f'Moving {name} {version} to {repo} {target_environment}')

        if package['environment'] == target_environment:
            print(f'Already in {repo} {target_environment}\n')
            continue

        print(dist_move(name, version, repo, package['environment'], target_environment))


@dist_group.command('copy')
@argument('name')
@argument('version')
@argument('target_repository', metavar='REPOSITORY')
@option('--from', 'source_repository')
@option('-e', '--env', 'target_environment', default='stable')
def copy_command(name, version, source_repository, target_repository, target_environment):
    """Copy package to the specified repository."""
    if not target_environment:
        raise ClickException('Option --env must be specified.')

    packages = dist_find(name, source_repository, version=version)
    if not packages:
        raise ClickException(f'No {name} packages with version {version} were found')

    package = packages[0]

    print(f'Copying {name} {version} from {source_repository} to {target_repository} {target_environment}')

    if dist_find(name, target_repository, version=version):
        print(f'Already in {target_repository}\n')
    else:
        print(dist_copy(name, version, package['repository'], target_repository, target_environment))
