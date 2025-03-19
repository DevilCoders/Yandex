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

logger = logging.getLogger('consumption_predictor_vh_workflow')



@timing
@vh.module(container=str, yt_token=vh.Secret, yql_token=vh.Secret, input=vh.File, script=str, output=vh.mkoutput(vh.File))
def run_script(container, yt_token, yql_token, input, script, output):
    joblayer = vh.op(id='6ae762d7-2cda-49d7-b976-99cd40d281c6')
    return joblayer(input=input, output=output, _name=script,
            script=f'tar xvf input &&  PYTHONPATH=./src:${{PYTHONPATH}} &&  python3.7 {script}',
            yt_token=yt_token, yql_token=yql_token, job_layer=[container])
     
def get_package(local_script=True):
    local_path = join(Path.home(), 'arc/arcadia/cloud/analytics/python/')
    if local_script:
        archive_name = join(local_path, 'cloud-antifraud/deploy.tar')
        package_dir = join(local_path, 'cloud-antifraud')
        create_archive(package_dir, ['src/', 'scripts/fraud_detection.py', 'config'], archive_name)
        tools_dir = join(local_path, 'lib/clan_tools')
        create_archive(tools_dir, ['src/'], archive_name, append=True)
        archive = vh.File(archive_name)
    else:
        package= vh.arcadia_folder('cloud/analytics/python/cloud-antifraud/')
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
    logger.info('Updating prod workflow' if update_prod else 'New workflow was created')
    logger.info('Workflow will start' if (not no_start) else 'Workflow will not start')



    package = get_package(local_src)
    experiment = run_script(container='378c7a62-50a5-4017-85e1-dd9f6cf0217f', 
               yt_token='robot-clanalytics-yt',
               yql_token='robot-clanalytics-yql',
               input=package,
               script=f'scripts/fraud_detection.py {"--write" if write else ""}').output
   

    keeper = vh.run(wait=(not no_start) and (not update_prod),  
                    start=(not no_start) and (not update_prod),
                    workflow_guid = '894604f8-c858-43c9-a908-4312f60e080a' if update_prod else None,
                    label='[cloud_antifraud] data exctraction',
                    project='cloud_antifraud',
                    quota='coud-analytics', backend=vh.NirvanaBackend())



if __name__ == '__main__':
    main()