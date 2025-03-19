from click import group, option, pass_context
from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.dbaas.internal.metadb.component import get_component_version, update_component_version


@group('component-version')
def component_group():
    """Cluster component version management commands."""
    pass


@component_group.command(name='get')
@option('-c', '--cluster', 'cluster_id')
@option('-v', '--component', 'component')
@pass_context
def get_cluster_component_version(ctx, cluster_id, component):
    """Get cluster component version."""

    print_response(ctx, get_component_version(ctx, cluster_id, component))


@component_group.command(name='update')
@option('-c', '--cluster', 'cluster_id')
@option('-v', '--component', 'component')
@option('-m', '--minor_version', 'minor_version')
@option('-p', '--package_version', 'package_version')
@pass_context
def update_cluster_component_version(ctx, cluster_id, component, minor_version, package_version):
    print_response(
        ctx,
        update_component_version(
            ctx,
            cluster_id=cluster_id,
            component=component,
            minor_version=minor_version,
            package_version=package_version,
        ),
    )
