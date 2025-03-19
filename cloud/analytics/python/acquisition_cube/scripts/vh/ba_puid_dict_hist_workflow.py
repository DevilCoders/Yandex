import vh
from clan_tools.vh.workflow import run_ch_script_1, run_ch_script_2, get_package
from clan_tools.utils.timing import timing
from clan_tools.logging.logger import default_log_config
import click
import logging.config
logging.config.dictConfig(default_log_config)
import json
logger = logging.getLogger(__name__)
from textwrap import dedent

@timing
@click.command()
@click.option('--prod', is_flag=True)
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=True,  prod=False, no_start=False, update_prod=False):
    logger.debug('Starting graph')
    logger.info('Updating prod workflow' if update_prod else 'New workflow was created')
    logger.info('Workflow will start' if (not no_start) else 'Workflow will not start')    
    package = get_package(package_path='acquisition_cube', local_script=local_src, 
                          files_to_copy=['scripts/',  'setup.py'])
    container='af8fd4f2-5248-45fd-b719-5f858d067ead', 
          
    
    

    run_job_op = vh.op(id='0a229ffc-9f79-4758-8f85-b76d361c1b74')

    def extract_and_run(script):
            return f'tar xvf input &&  PYTHONPATH=./src:${{PYTHONPATH}} && python2.7 {script} {"--prod" if prod else ""}'

    
    def run_job(script):
            target = run_job_op(input=package, _name=script,
                    script=extract_and_run(script),
                    yt_token='robot-clanalytics-yt', job_layer=container)
            return target

    run_job('scripts/ba_puid_dict_hist.py')


    keeper = vh.run(wait=False,  
                    start=(not no_start) and (not update_prod),
                    workflow_guid = 'b6f71174-086f-4099-9a2f-77c5c1827b09' if update_prod else None,
                    project='acquisition_cube',
                    quota='coud-analytics', 
                    # quota='external-activities',
                    label='ba_puid_dict_hist_acquisition_cube',
                    backend=vh.NirvanaBackend())

   



if __name__ == '__main__':
    main()
