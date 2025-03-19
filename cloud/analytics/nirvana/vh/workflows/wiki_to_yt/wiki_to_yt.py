# ACTUALIZE????

import vh
import requests
import yt.wrapper as yt
import click
import os

from cloud.analytics.nirvana.vh.config import DEFAULT_RUN_CONFIG, REACTOR_PREFIX, REACTOR_OWNER
from cloud.analytics.nirvana.vh.utils.reactor import ReactorWrapper
import reactor_client as r


REACTION_NAME = 'wiki_to_yt'


@vh.lazy(
    object,
    table_columns=vh.mkinput(str),
    wiki_uri=vh.mkinput(str),
    wiki_token=vh.Secret,
    dst_yt_table_prefix=vh.mkinput(str),
    dst_yt_cluster=vh.mkinput(str),
    yt_token=vh.Secret,
    ts=vh.mkinput(str),
)
def wiki_to_yt(table_columns, wiki_uri, wiki_token, dst_yt_table_prefix, dst_yt_cluster, yt_token, ts):
    def data_iterator(json_items, column_names):
        items = [[c['raw'] for c in row]
                 for row in json_items['data']['rows']]
        for item in items:
            yield {k: v for k, v in zip(column_names, item)}

    wiki_url = 'https://wiki-api.yandex-team.ru/_api/frontend/%s/.grid' \
               % wiki_uri
    r = requests.get(
        wiki_url,
        headers={
            'Authorization': 'OAuth %s' % wiki_token.value
        }
    )

    yt.config['token'] = yt_token.value
    yt.config['proxy']['url'] = dst_yt_cluster
    yt.write_table(
        dst_yt_table_prefix,
        data_iterator(r.json(), table_columns.split(','))
    )


# TODO(syndicut): Move all boilerplate to decorator for main?
@click.command()
@click.option('--local', default=False, help='Launch on Valhalla local backend.', is_flag=True)
@click.option('--project', help='Nirvana project to launch graph in.')
@click.option('--option', '-o', help='Global options for Valhalla local backend', multiple=True, type=(str, str))
@click.option('--reactor-prefix', help='Prefix for namespace in Reactor.', default=REACTOR_PREFIX)
@click.option('--reactor-owner', help='Owner for reaction.', default=REACTOR_OWNER)
def main(local, project, option, reactor_prefix, reactor_owner):
    run_config = DEFAULT_RUN_CONFIG

    if local:
        run_config['backend'] = vh.LocalBackend()
        for key, value in option:
            run_config['global_options'][key] = value

        for env_var, value in os.environ.items():
            if env_var.startswith('VH_'):
                secret_name = env_var[3:].lower()
                run_config['secrets'][secret_name] = value

    if project:
        run_config['project'] = project

    table_columns = vh.add_global_option('table_columns', 'string', help='Comma separated table columns', default='crm_client_name,billing_account_id,sales,segment')
    wiki_uri = vh.add_global_option('wiki_uri', 'string', default='cloud-bizdev/Enterprise-ISV-clients/Spisok-Ent/ISV/', help='URI to wiki page with table')
    dst_yt_table_prefix = vh.add_global_option('dst_yt_table_prefix', 'string', default='//home/cloud_analytics/import/wiki/clients_segments', help='Prefix for destination table in YT')
    dst_yt_cluster = vh.add_global_option('hahn', 'string', default='hahn', help='YT cluster for destination table')

    wiki_token_name = vh.add_global_option('wiki_token_name', 'secret', default='cloud_analytics_wiki_token')
    yt_token_name = vh.add_global_option('yt_token_name', 'secret', default='robot-clanalytics-yt-yt-token')
    ts = vh.add_global_option('ts', 'string', help='Timestamp for deterministic workaround', default="0")

    wiki_to_yt(table_columns, wiki_uri, wiki_token_name, dst_yt_table_prefix, dst_yt_cluster, yt_token_name, ts)

    # TODO(syndicut): Do not create a new workflow for each run
    keeper = vh.run(**run_config)

    reaction_path = '/'.join((reactor_prefix, REACTION_NAME))
    info = keeper.get_workflow_info()
    reactor_wrapper = ReactorWrapper()

    builder = r.reaction_builders.NirvanaReactionBuilder()

    builder.trigger_by_cron('0 0 */6 * * ? *')

    builder.set_source_graph(instance_id=info.workflow_instance_id)
    builder.set_owner(reactor_owner)
    builder.set_reaction_path(reaction_path)

    builder.set_global_param_to_expression("ts", 'return Time.unixNowAsString();')

    reactor_wrapper.replace_reaction(builder, local)


if __name__ == '__main__':
    main()
