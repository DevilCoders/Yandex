import vh
from datetime import datetime
from vh.frontend.nirvana import OpPartial
from clan_tools.logging.logger import default_log_config
import logging.config
from functools import partial
from clan_tools.utils.timing import timing
from clan_tools.vh.operations import get_mr_dir
import click

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)

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


    yql2 = run_yql_script(path='src/support_knowladge_base/yql/collect_train_val_data.sql', version=2)
    yql1 = run_yql_script(path='src/support_knowladge_base/yql/all_tickets.sql', version=1)
    
    results = partial(
        vh.op(id='8878a0a5-2ac3-4719-87f8-bd5da81d3e06'),
        mr_account=write_conf['mr_account'],
        yt_token=write_conf['yt_token'],
        common_training_params__batch_size_per_gpu=4
        )(train_table=yql2.output1, val_table=yql2.output2)

    model = partial(
        vh.op(id='e1a6c2a5-3246-4a72-9945-2df3c55c8677'),
        pulsar_token='albina-volk-pulsar-token')(wizard_options=results.output)

    mr_dir = get_mr_dir(_options={'path':'//home/cloud_analytics/ml/support_knowladge_base/model'}).mr_directory
    partial(
        vh.op(id='e24705ea-edfd-403d-bfa0-0d49ffe6775c'),
        mr_account=write_conf['mr_account'],
        yt_token=write_conf['yt_token'],
        dst_name=str(datetime.now())[:10]
    )(source=model['large.npz'], dst_dir=mr_dir)


    embed = partial(
        vh.op(id='ff992e2a-e4ac-4ec6-8b78-45e8b4064a7f'),
        model_head='query',
        column_to_embed='query',
        yt_token=write_conf['yt_token'],
        mr_account=write_conf['mr_account'],
        )(finetuned_model=model['large.npz'], input_table=yql1.output1)


    run_yql_script(path='src/support_knowladge_base/yql/normalize_results.sql', version=1, input1=embed.output)

    vh.run(
        wait=False,
        keep_going=True,
        start=with_start,
        workflow_guid=('8859b751-836b-4f90-b58a-8ae9dca5ee24' if is_prod else None),
        quota='coud-analytics',
        backend=vh.NirvanaBackend()
    )


if __name__ == '__main__':
    main()
