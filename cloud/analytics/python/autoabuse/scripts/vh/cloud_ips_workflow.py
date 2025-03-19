import vh
from clan_tools.vh.workflow import  get_package
from clan_tools.utils.timing import timing
from clan_tools.logging.logger import default_log_config
import click
import logging.config
logging.config.dictConfig(default_log_config)
import json
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=True,  write=False, no_start=False, update_prod=False):
        logger.debug('Starting graph')
        logger.info('Updating prod workflow' if update_prod else 'New workflow was created')
        logger.info('Workflow will start' if (not no_start) else 'Workflow will not start')    
        package = get_package(package_path='autoabuse', local_script=local_src, 
                                files_to_copy=['scripts/'])
        container = '330176a3-9b7d-404b-acab-be9bc91af303'
        run_small_job_op = vh.op(id='994ff51d-dc71-4c45-80d5-fdddd32fc482')

        def extract_and_run(script):
                return f'tar xvf input &&  PYTHONPATH=./src:${{PYTHONPATH}} && python3.7 {script} {"--write" if write else ""}'

        
        def run_job(script):
                target = run_small_job_op(input=package, _name=script,
                        script=extract_and_run(script),
                        yql_token='robot-clanalytics-yql', job_layer=[container])
                return target

        latest_time_target = run_job('scripts/network_stats_latest_time.py')

        with vh.wait_for(latest_time_target):
                run_job('scripts/cloud_instance_ips.py')



        keeper = vh.run(wait=False,  
                        keep_going=True,
                        start=(not no_start) and (not update_prod),
                        workflow_guid = 'e2322793-4a85-468f-957e-c1c75fdc54ac' if update_prod else None,
                        project='autoabuse',
                        quota='coud-analytics', 
                        # quota='external-activities',
                        label='cloud_ips',
                        backend=vh.NirvanaBackend())

   



if __name__ == '__main__':
    main()
