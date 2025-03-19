import vh
from clan_tools.utils.conf import read_conf
import logging.config
from clan_tools.utils.dict import DictObj
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package, prepare_dependencies
import click
from textwrap import dedent
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.vh.operations import (spark_op,
                                      pulsar_add_instance_op,
                                      extract_metrics, run_job_op)
from clan_tools.secrets.Vault import Vault
Vault().get_secrets()

config = read_conf('config/logger.yml')
logging.config.dictConfig(config)

logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--workflow', help='use main, hypothesis or test for parallel run of models for hypothesis testing')
def main(local_src=False, no_start=False, workflow='main'):
    dataset_name = 'CLOUDANA-1418'
    model_name = 'CLOUDANA-1353'

    update_prod = (workflow in ['main', 'hypothesis'])
    package = get_package(package_path='ml/scoring/consumption_predictor', local_script=local_src,
                          files_to_copy=['config/', 'src/', 'scripts/'])

    root_dir = f'//home/cloud_analytics{"" if update_prod else "/test"}'
    model_dir = 'targets_from_mass' + "/" + dataset_name + "/" + model_name

    paths = DictObj(
        train_features_dir=f'{root_dir}/smb/{model_dir}/train_features',
        predict_features_dir=f'{root_dir}/smb/{model_dir}/predict_features',
        predicts_dir=f'{root_dir}/smb/{model_dir}',
        deps_dir=f'{root_dir}/smb/{model_dir}/deps',
        crm_results_path=f'{root_dir}/smb/{model_dir}/crm_leads')

    YTAdapter().create_paths(paths.values())

    extract_driver_filename = 'extract_features_spark.py'
    crm_driver_filename = 'add_preds_to_crm.py'
    dependencies = prepare_dependencies(
        package, paths.deps_dir, [extract_driver_filename, crm_driver_filename])

    with vh.wait_for(*dependencies):

        with vh.wait_for(spark_op(_name="Extract features",
                                  spyt_deps_dir=paths.deps_dir,
                                  spyt_driver_filename=extract_driver_filename,
                                  spyt_driver_args=[
                                        f"--train_features_dir {paths.train_features_dir}",
                                        f"--predict_features_dir {paths.predict_features_dir}"
                                  ])):
            experiment = run_job_op(yt_token='robot-clanalytics-yt',
                                    input=package,
                                    _name='Run Model',
                                    script=dedent('scripts/run_model.py ' +
                                                  f'--train_features_dir {paths.train_features_dir} ' +
                                                  f'--predict_features_dir {paths.predict_features_dir} ' +
                                                  f'--predicts_dir {paths.predicts_dir} ' +
                                                  f'--model_name {model_name}'
                                                  )).output

    metrics = extract_metrics(inp=experiment, field='metrics')

    pulsar_add_instance_op(metrics.out,
                           model_name=model_name,
                           dataset_name=dataset_name,
                           pulsar_token='robot-clanalytics-pulsar',
                           tags=['cloudana_332', '30k'])

    with vh.wait_for(metrics):
        spark_op(_name="Save leads to CRM",
                 spyt_deps_dir=paths.deps_dir,
                 spyt_driver_filename=crm_driver_filename,
                 spyt_driver_args=[
                     f"--predicts_dir {paths.predicts_dir} " +
                     f"--crm_path {paths.crm_results_path}"
                 ])

    workflows_map = {
        'main': '9eb64db1-cd23-48f5-a6e1-a65ed040d14c',
        'hypothesis': '4e6d46a3-08a0-4dd2-a207-932ae96cda5c',
        'test': '894b7042-cfd5-4752-a506-4a28ad3338ef'
    }

    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           workflow_guid=workflows_map[workflow],
           label=f'[cloudana_332] 30k {workflow}',
           description='ML based prediction of potential candidates for 30k consumption',
           project='cloudana_332',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
