import vh
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import current_date_op, run_small_job_op

from clan_tools.utils.timing import timing
from clan_tools.utils.conf import read_conf
import click
import logging.config
from textwrap import dedent

config = read_conf('config/logger.yml')
logging.config.dictConfig(config)
logger = logging.getLogger(__name__)



@vh.module(
    out=vh.mkoutput(vh.File),
    package=vh.mkinput(vh.File),
    current_date=vh.mkinput(vh.File),
    script=vh.mkoption(str),
    max_ram=vh.mkoption(int, default=8 * 1024),
    ttl=vh.mkoption(int, default=72 * 3600),
    cpu_guarantee=vh.mkoption(int, default=3200)
)
def support_summonee_stats(out, package, script, current_date, max_ram, ttl, cpu_guarantee):
    
    yt_token_secret = vh.get_yt_token_secret()
    tracker_token = vh.add_global_option('tracker_token', vh.Secret, required=True)
    clickhouse_token = vh.add_global_option('clickhouse_token', vh.Secret, required=True)
    ch_username = vh.add_global_option('ch_username', str, required=True)
    mdb_oauth_token = vh.add_global_option('mdb_oauth_token', vh.Secret, required=True)
                   
    vh.tgt(out, package, current_date,
            recipe= 'tar xvf {{ package }} '
                  + '&& cat {{ current_date }} '
                  + '&& PYTHONPATH=./src:$PYTHONPATH '
                  + 'python3.7 scripts/support/{{ script }} '
                  + '{{ yt_token_secret }} '
                  + '{{ tracker_token }} '
                  + '{{ clickhouse_token }} '
                  + '{{ mdb_oauth_token }} '
                  + '{{ ch_username }} '
                  + '&& cat output.json > {{ out }}',
            container=vh.Porto([vh.data_from_str('ad5be553-cbcd-4d45-a8e6-e478a3d13dd6', data_type='binary')]),
            hardware_params=vh.HardwareParams(max_ram=max_ram, ttl=ttl, cpu_guarantee=cpu_guarantee)
          )


@timing
@click.command()
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=True,  no_start=False, update_prod=False):
       
        package = get_package(package_path='python/dashboards_jobs', local_script=local_src, 
                                files_to_copy=['config/', 'scripts/'])
       
        current_date = current_date_op().output
        summonee_stats = support_summonee_stats(package=package, script='support_summonee_stats.py', current_date=current_date)
        
        with vh.wait_for(summonee_stats):
            run_small_job_op(input=package, _name='support_tickets_count',
                    script='scripts/support/support_tickets_count.py',
                    yt_token='robot-clanalytics-yt')

        vh.run(wait=False,  
               keep_going=True,
               start=(not no_start) and (not update_prod),
               global_options={
                   'tracker_token':'robot-clanalytics-yt_startrek_token',
                   'clickhouse_token':'robot-clanalytics-yt_clickhouse_token',
                   'ch_username': 'admin',
                   'mdb_oauth_token': 'robot-clanalytics-yt_mdb_oauth_token'
               },
               yt_token_secret='robot-clanalytics-yt', 
               workflow_guid = '014b1ba2-fcd1-4be7-836c-7449e552243b' if update_prod else None,
               project='clan_dashboards',
               quota='coud-analytics', 
               label='support_summonee_stats',
               backend=vh.NirvanaBackend())

   



if __name__ == '__main__':
    main()
