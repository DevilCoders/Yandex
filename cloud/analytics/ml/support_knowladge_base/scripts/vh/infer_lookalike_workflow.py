import vh
from datetime import datetime
from vh.frontend.nirvana import OpPartial
from clan_tools.logging.logger import default_log_config
import logging.config
from functools import partial
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.utils.timing import timing
from clan_tools.vh.operations import get_mr_dir
import click

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)

yt_adapter = YTAdapter()
write_conf = dict(
    yt_token='robot-clanalytics-yt',
    yql_token='robot-clanalytics-yql',
    mr_default_cluster='hahn',
    mr_account='cloud_analytics',
)


def run_yql_script(path:str, version=1, input1=None) -> OpPartial:
    if version==2:
        op_id='4494caaf-1915-4e8b-ad9f-f4a1fbe2c1ae'
    else:
        op_id='7e1a831b-8f95-48fb-8472-4539f0261da9'

    with open(path, 'r') as fin: 
        request =  fin.read()
    return partial(
        vh.op(id=op_id),
        mr_account=write_conf['mr_account'],
        yt_token=write_conf['yt_token'],
        yql_token=write_conf['yql_token'])(request=request, input1=input1)

@timing
@click.command()
@click.option('--local_src', is_flag=True, default=False)
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--with_start', is_flag=True, default=False)
def main(local_src: bool = False,  is_prod: bool = False, with_start: bool = False) -> None:
    logger.debug('Starting graph')
    logger.info('Updating production workflow' if is_prod else 'Testing workflow')
    logger.info('Workflow will start' if with_start else 'Workflow will not start')

    yql1 = run_yql_script(path='src/support_knowladge_base/yql/new_tickets.sql', version=1)
    
    folder='//home/cloud_analytics/ml/support_knowladge_base/model'
    last_model = folder + '/' + max(yt_adapter.yt.list(folder))
    get_mr_file = partial(
        vh.op(id='ec4a8561-87ff-4095-afd5-3ca4f6226ea8'),
        cluster=write_conf['mr_default_cluster'],
        yt_token=write_conf['yt_token'])(_options={'path':last_model})

    embed = partial(
        vh.op(id='ff992e2a-e4ac-4ec6-8b78-45e8b4064a7f'),
        model_head='query',
        column_to_embed='query',
        yt_token=write_conf['yt_token'],
        mr_account=write_conf['mr_account'],
    )(finetuned_model=get_mr_file.mr_file, input_table=yql1.output1)

    run_yql_script(path='src/support_knowladge_base/yql/prepare_results.sql', version=1, input1=embed.output)

    
    vh.run(
        wait=False,
        keep_going=True,
        start=with_start,
        workflow_guid=('bfee01a6-f1c6-44e7-b446-30237d5a4edd' if is_prod else None),
        quota='coud-analytics',
        backend=vh.NirvanaBackend()
    )


if __name__ == '__main__':
    main()
