import vh
from clan_tools.vh.workflow import get_package
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

        

        key_partners_script = 'scripts/key_partners/key_partners.py'
        key_partners_target = run_dash_job(input=package, _name=key_partners_script,
                script=extract_and_run(key_partners_script),
                yql_token='robot-clanalytics-yql', job_layer=[container])

        with vh.wait_for(key_partners_target):
                key_partners_account_owners_script = 'scripts/key_partners/key_partners_account_owners.py'
                key_partners_account_owners_target = run_dash_job(input=package, _name=key_partners_account_owners_script,
                        script=extract_and_run(key_partners_account_owners_script),
                        yql_token='robot-clanalytics-yql', job_layer=[container])

        with vh.wait_for(key_partners_account_owners_target):
                key_partners_opportunities_script = 'scripts/key_partners/key_partners_opportunities.py'
                key_partners_opportunities_target = run_dash_job(input=package, _name=key_partners_opportunities_script,
                        script=extract_and_run(key_partners_opportunities_script),
                        yql_token='robot-clanalytics-yql', job_layer=[container])

        



        dim_crm_account_script = 'scripts/cdm/dim_crm_account.py'
        dim_crm_account_target = run_dash_job(input=package, _name=dim_crm_account_script,
                script=extract_and_run(dim_crm_account_script),
                yql_token='robot-clanalytics-yql', job_layer=[container])

        with vh.wait_for(dim_crm_account_target):
                var_partners_managers_script = 'scripts/var/var_partners_managers.py'
                var_partners_managers_target = run_dash_job(input=package, _name=var_partners_managers_script,
                        script=extract_and_run(var_partners_managers_script),
                        yql_token='robot-clanalytics-yql', job_layer=[container])

        with vh.wait_for(var_partners_managers_target):
                var_dashboard_script = 'scripts/var/var_dashboard.py'
                var_dashboard_target = run_dash_job(input=package, _name=var_dashboard_script,
                        script=extract_and_run(var_dashboard_script),
                        yql_token='robot-clanalytics-yql', job_layer=[container])

        with vh.wait_for(var_dashboard_target):
                var_referrals_script = 'scripts/var/var_referrals.py'
                run_dash_job(input=package, _name=var_referrals_script,
                        script=extract_and_run(var_referrals_script),
                        yql_token='robot-clanalytics-yql', job_layer=[container])
                        

        keeper = vh.run(wait=False,  
                        keep_going=True,
                        start=(not no_start) and (not update_prod),
                        workflow_guid = 'eb33f831-e420-4753-9326-d097e1e6ec3d' if update_prod else None,
                        project='clan_dashboards',
                        quota='coud-analytics', 
                        # quota='external-activities',
                        label='dashboards',
                        backend=vh.NirvanaBackend())

   



if __name__ == '__main__':
    main()
