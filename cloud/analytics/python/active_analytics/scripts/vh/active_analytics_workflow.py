import vh
from clan_tools.utils.archive import create_archive
from os.path import join
from pathlib import Path
from clan_tools.utils.conf import read_conf
import logging.config
from clan_tools.utils.timing import timing
import click
config = read_conf('config/logger.yml')
logging.config.dictConfig(config)

logger = logging.getLogger(__file__)

@timing
@vh.module(container=str, ch_token=vh.Secret, tracker_token=vh.Secret, input=vh.File, script=str)
def run_script(container, ch_token, tracker_token, input, script):
    joblayer = vh.op(id='826277f1-b222-428d-b7ed-cd6c5d02b294')
    joblayer(input=input, _name=script,
            script=f'tar xvf input &&  PYTHONPATH=./src:${{PYTHONPATH}} &&  python3.7 {script}',
            ch_token=ch_token, tracker_token=tracker_token, job_layer=[container])

def get_package(local_script=True):
    local_path = join(Path.home(), 'arc/arcadia/cloud/analytics/python/')
    if local_script:
        archive_name = join(local_path, 'active_analytics/deploy.tar')
        package_dir = join(local_path, 'active_analytics')
        create_archive(package_dir, ['src', 
                                     'scripts/low_consumptions.py', 
                                     'scripts/daily_consumptions.py',
                                     'config'], archive_name)
        tools_dir = join(local_path, 'lib/clan_tools')
        create_archive(tools_dir, ['src/'], archive_name, append=True)
        archive = vh.File(archive_name)
    else:
        package= vh.arcadia_folder('cloud/analytics/python/active_analytics/')
        tools = vh.arcadia_folder('cloud/analytics/python/lib/clan_tools')
        merge_archives_op = vh.op(id='31d7bcd4-c5f4-4398-9bf1-070e5466c219') 
        archive =  merge_archives_op(tar_archives=[package, tools])
    return archive


@timing
@click.command()
@click.option('--write', is_flag=True)
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=False, write=False, no_start=False, update_prod=False):
    logger.debug('Starting graph')
    package = get_package(local_src)
    conf = dict(ch_token = 'bakuteev-clickhouse-token',
                tracker_token = 'robot-clanalytics-startrek',
                container='d52ed8c4-da59-4214-a355-8bb29a929c7d',
                input=package)
    
    # run_script(script=f'scripts/low_consumptions.py {"--write" if write else ""}', **conf)
    run_script(script=f'scripts/daily_consumptions.py {"--write" if write else ""}', **conf)

    keeper = vh.run(wait=(not no_start) and (not update_prod),  
                    start=(not no_start) and (not update_prod),
                    workflow_guid ='f672f567-33a4-4532-a3ee-f1203bccea45' if update_prod else None,
                    project='alerting',
                    quota='coud-analytics', 
                    label='alerting',
                    backend=vh.NirvanaBackend())



if __name__ == '__main__':
    main()