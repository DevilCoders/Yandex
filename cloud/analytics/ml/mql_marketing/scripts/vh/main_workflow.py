import logging.config

import click
import pandas as pd

import vh
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import run_job_op, run_yql_script


logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--local_src', is_flag=True, default=False)
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--with_start', is_flag=True, default=False)
def main(local_src,  is_prod, with_start):
    logger.debug('Starting graph')
    logger.info('Updating production workflow' if is_prod else 'Testing workflow')
    logger.info('Workflow will start' if with_start else 'Workflow will not start')

    # common operation of folder loading with correction: we write config to src
    package = get_package(package_path='ml/mql_marketing', local_script=local_src, files_to_copy=['src/', 'scripts/'])
    ind_script = 'src/mql_marketing/yql/MQL_individual.sql'
    comp_script = 'src/mql_marketing/yql/MQL_company.sql'
    mal_script = 'src/mql_marketing/yql/MQL_mal.sql'
    spax_script = 'scripts/add_spax.py'

    # script for individual puids
    df_droplist_like = pd.read_csv('src/mql_marketing/company_lists/droplist_like.csv')
    droplist_like = '\t\t' + '\n\t\t'.join("AND `company_name` NOT LIKE '%" + df_droplist_like.values.ravel() + "%'")
    df_droplist_isin = pd.read_csv('src/mql_marketing/company_lists/droplist_isin.csv', keep_default_na=False)
    droplist_isin = "\n\t\tAND `company_name` NOT IN (" + '\n\t\t\t' + ',\n\t\t\t'.join("'"+df_droplist_isin.values.ravel()+"'") + "\n\t\t)"
    with open(ind_script, 'r') as fin:
        query = fin.read()
        query = query + droplist_like + droplist_isin + '\n\tORDER BY `date`, `puid`\n;\n'

    op_by_puids = run_yql_script(_name='MQL Individual', query=query, yql_token='robot-clan-pii-yt-yql_token')

    with vh.wait_for(op_by_puids):
        spax_op = run_job_op(_name='Add spax', input=package, script=spax_script, max_ram=32*1024, job_layer='base-0.1',
                             yql_token='robot-clan-pii-yt-yql_token', yt_token='robot-clan-pii-yt-yt_token')

    with vh.wait_for(spax_op):
        op_by_companies = run_yql_script(_name='MQL Companies', path=comp_script, yql_token='robot-clan-pii-yt-yql_token')

    with vh.wait_for(op_by_companies):
        run_yql_script(_name='MQL MAL-list', path=mal_script, yql_token='robot-clan-pii-yt-yql_token')

    workflow_id = 'c494a1ec-e870-4ab9-ad36-d1a82a12bd95' if is_prod else '1d85fda7-aae7-4dd1-81f3-440ef9a0a4a3'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='mql_marketing',
           quota='coud-analytics',
           label=f'MQL marketing ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
