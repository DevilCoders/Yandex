from click import argument, group, pass_context

from cloud.mdb.cli.dbaas.internal.intapi import perform_operation_rest


@group('dictionary')
def dictionary_group():
    """Commands to manage ClickHouse external dictionaries."""
    pass


@dictionary_group.command('create')
@argument('cluster_id', metavar='CLUSTER')
@argument('name')
@pass_context
def create_dictionary_command(ctx, cluster_id, name):
    """Create ClickHouse external dictionary."""
    perform_operation_rest(
        ctx,
        'PATCH',
        'clickhouse',
        f'clusters/{cluster_id}',
        data={
            'configSpec': {
                'clickhouse': {
                    'config': {
                        'dictionaries': [
                            {
                                'name': name,
                                'layout': {
                                    'type': 'FLAT',
                                },
                                'httpSource': {
                                    'url': 'https://localhost:8443/?query=select%201%2C%27test%27',
                                    'format': 'TSV',
                                },
                                'fixedLifetime': 300,
                                'structure': {
                                    'id': {
                                        'name': 'id',
                                    },
                                    'attributes': [
                                        {
                                            'name': 'text',
                                            'type': 'String',
                                            'nullValue': '',
                                        }
                                    ],
                                },
                            }
                        ],
                    },
                },
            },
        },
    )
