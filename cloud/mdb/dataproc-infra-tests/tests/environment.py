"""
Behave entry point.

For details of env bootstrap, see env_control
"""
import logging
import os
import json
import signal
from traceback import print_exc

from tests import env_control, logs
from tests.helpers import compute_driver, database, deploy_api


def fill_context(context):
    """
    Prepare context before features.
    """
    # Add static variables to context
    put_to_context = context.conf.get('put_to_context', {})
    for key, val in put_to_context.items():
        setattr(context, key, val)

    endpoint = context.conf['s3']['endpoint_url']
    bucket = context.conf['s3']['bucket_name']
    setattr(context, 'agent_version', compute_driver.get_agent_version(context.conf))
    setattr(context, 'agent_repo', f'https://{endpoint}/{bucket}/agent')


def before_all(context):
    """
    Prepare environment for tests.
    """
    logs.init_logger()

    context.state = env_control.start()
    context.conf = context.state['config']
    if context.conf.get('fail_fast', False):
        context.config.stop = True

    fill_context(context)
    compute_driver.put_ssh_private_key(context.state, context.conf)

    if not context.conf.get('disable_cleanup', False):
        signal.signal(signal.SIGINT, lambda *args: env_control.stop(state=context.state))
        signal.signal(signal.SIGTERM, lambda *args: env_control.stop(state=context.state))
        compute_driver.cleanup_environment(context)
    context.tunnels_pool = {}
    return


def before_scenario(context, _):
    """
    Cleanup function executing per feature scenario.
    """
    pass


def before_feature(context, _):
    """
    Cleanup function executing per feature.
    """
    pass


def after_step(context, step):
    """
    Save logs after failed step
    """
    if step.status == 'failed':
        if context.conf.get('dump_logs_and_dbs', True):
            try:
                logs.save_logs(context)
            except Exception:
                print('Unable to save logs')
                print_exc()

        if context.config.userdata.getbool('debug'):
            try:
                import ipdb as pdb
            except ImportError:
                import pdb
            pdb.post_mortem(step.exc_traceback)

        if context.config.userdata.getbool('skip-dependent-scenarios'):
            if 'dependent-scenarios' in context.feature.tags:
                for scenario in context.feature.scenarios:
                    if scenario.status == 'untested':
                        scenario.skip('Skip due "%s" fail' % context.scenario.name)


def list_error_jobs(conf):
    shipments_data = deploy_api.list_shipments(conf)
    for shipment in shipments_data.get('shipments'):
        if shipment.get('status') != 'error':
            continue
        if not shipment.get('id'):
            continue
        jobs_data = deploy_api.list_shipment_jobs(conf, shipment['id'])
        for job in jobs_data.get('jobs', []):
            if job.get('status') != 'error':
                continue
            for fqdn in shipment['fqdns']:
                yield fqdn, job['extId']


def dump_jobresults(conf, path):
    os.makedirs(path, exist_ok=True)
    filename = os.path.join(path, 'failed_jobs.log')
    with open(filename, 'w') as logfile:
        for fqdn, job_id in list_error_jobs(conf):
            job_results = deploy_api.get_job_results(conf, fqdn, job_id)
            for result in job_results.get('jobResults', []):
                if not isinstance(result, dict):
                    continue
                if result.get('status') != 'failure':
                    continue
                logfile.write(f'{fqdn} - {job_id}:\n')
                json.dump(result, logfile, indent=2)
                logfile.write('\n')


def after_all(context):
    """
    Clean up and collect info.
    """
    if context.conf.get('dump_logs_and_dbs', True):
        try:
            dump_dir = os.path.join(context.conf['staging_dir'], 'redis')
            database.redis_dump_json(context, dump_dir)
        except Exception:
            print('Unable to dump redis')
            print_exc()
        try:
            csv_dir = os.path.join(context.conf['staging_dir'], 'meta')
            database.pg_dump_csv(context, csv_dir)
        except Exception:
            print('Unable to dump csv')
            print_exc()
        try:
            dump_dir = os.path.join(context.conf['staging_dir'], 'jobresults')
            if context.state.get('persistent', {}).get('managed'):
                dump_jobresults(context.conf, dump_dir)
            else:
                print('Collecting jobresults skipped')
        except Exception:
            print('Unable to dump failed jobs from deploy-api')
            print_exc()

    if (context.failed and not context.aborted) or context.conf.get('disable_cleanup', False):
        logging.warning('Remember to run `make clean` after you done')
        return

    if hasattr(context, 'tunnels_pool'):
        for _, tunnel in context.tunnels_pool.items():
            try:
                tunnel.stop()
            except Exception:
                print('Unable to close tunnel')
                print_exc()

    compute_driver.cleanup_environment(context)
    env_control.stop(state=context.state)
