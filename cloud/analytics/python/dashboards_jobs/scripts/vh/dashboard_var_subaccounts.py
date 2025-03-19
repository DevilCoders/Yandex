import vh
from clan_tools.vh.workflow import run_ch_script_1, get_package
from clan_tools.utils.timing import timing
from clan_tools.utils.conf import read_conf
import click
import logging.config
config = read_conf('config/logger.yml')
logging.config.dictConfig(config)
import json
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--write', is_flag=True)
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=True,  write=False, no_start=False, update_prod=False):
        logger.debug('Starting graph')
        logger.info('Updating prod workflow' if update_prod else 'New workflow was created')
        logger.info('Workflow will start' if (not no_start) else 'Workflow will not start')    
        package = get_package(package_path='dashboards_jobs', local_script=local_src, 
                                files_to_copy=['config/', 'scripts/'])
        container = '330176a3-9b7d-404b-acab-be9bc91af303'
        run_dash_job = vh.op(id='0a229ffc-9f79-4758-8f85-b76d361c1b74')

        def extract_and_run(script):
                return f'tar xvf input &&  PYTHONPATH=./src:${{PYTHONPATH}} && python3.7 {script} {"--write" if write else ""}'

        support_script = 'scripts/var/var_subaccounts_consumption.py' 
        run_dash_job(input=package, _name=support_script,
                script=extract_and_run(support_script),
                yt_token='robot-clanalytics-yt', job_layer=[container])


        keeper = vh.run(wait=False,  
                        keep_going=True,
                        start=(not no_start) and (not update_prod),
                        workflow_guid = 'c75452f3-70c8-4030-8e12-8ecf874ca4cf' if update_prod else None,
                        project='clan_dashboards',
                        quota='coud-analytics', 
                        # quota='external-activities',
                        label='var_subaccounts',
                        backend=vh.NirvanaBackend())

if __name__ == '__main__':
    main()
