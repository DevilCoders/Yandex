import vh
from clan_tools.vh.workflow import get_package
from clan_tools.utils.timing import timing
from clan_tools.logging.logger import default_log_config
import click
import logging.config
logging.config.dictConfig(default_log_config)
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
                                files_to_copy=['scripts/', 'src/'])
        container = '330176a3-9b7d-404b-acab-be9bc91af303'
        run_job_op = vh.op(id='6ae762d7-2cda-49d7-b976-99cd40d281c6')

        def extract_and_run(script):
                return f'tar xvf input &&  PYTHONPATH=./src:${{PYTHONPATH}} && python3.7 {script} {"--write" if write else ""}'

        
        def run_job(script):
                target = run_job_op(input=package, _name=script,
                        script=extract_and_run(script),
                        yt_token='robot-clanalytics-yt',
                        tracker_token='robot-clanalytics-startrek',
                        yql_token='robot-clanalytics-yql', job_layer=[container])
                return target

        run_job('scripts/handle_tickets.py')


        keeper = vh.run(wait=False,  
                        keep_going=True,
                        start=(not no_start) and (not update_prod),
                        workflow_guid = '1af3767c-b510-4669-bd28-91f5b3e7272d' if update_prod else 'ad1425b5-13a1-415e-b349-392267e0acac',
                        project='autoabuse',
                        quota='coud-analytics', 
                        # quota='external-activities',
                        label='label_tickets' if update_prod else 'label_tickets_test',
                        backend=vh.NirvanaBackend())

   



if __name__ == '__main__':
    main()
