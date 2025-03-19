import vh
from os.path import join as path_join
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import extract_op, zip_op, spark_op, yt_write_file, pulsar_add_instance_op
from clan_tools.vh.operations import get_bin_from_yt, get_mr_table, catb_apply, run_small_job_op
import click

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--local_src', is_flag=True, default=False)
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--with_start', is_flag=True, default=False)
def main(local_src,  is_prod, with_start):
    logger.debug('Starting graph')
    logger.info('Updating production workflow' if is_prod else 'Testing workflow')
    logger.info('Workflow will start' if with_start else 'Workflow will not start')

    # common operation of folder loading with correction: we write config to src
    package = get_package(package_path='ml/scoring/consumption_predictor_v2', local_script=local_src, files_to_copy=['src/', 'scripts/'])

    # pathes in yt
    deps_path = '//home/cloud_analytics/ml/scoring/consumption_predictor_v2/deps'
    spyt_driver_sample_script = 'scripts/3_make_prod_sample_for_predict.py'
    spyt_driver_saving_script = 'scripts/3_save_prod_prediction.py'
    spyt_driver_metrics_script = 'scripts/3_calc_prod_metrics.py'

    # spyt params
    results_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/prod_results"
    features_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_features"
    temporary_calcs_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/last_predict_samples"
    target_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_target"
    report_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/metrics"

    # CatBoost: Apply params

    # thread #1: deploy.tar -> unpack bin of src folder -> compress to zip -> write it on yt
    src = extract_op(_name='Extract deps', archive=package, out_type='binary', path='src').binary_file
    src_zip = zip_op(_name='deps to ".zip"', input=src).output
    write_src_zip = yt_write_file(_name='deps.zip -> YT', file=src_zip, path=f'{deps_path}/dependencies.zip')

    # thread #2: deploy.tar -> unpack exec of driver -> write it on yt
    spyt_driver_sample = extract_op(_name='Extract sample script', archive=package, out_type='exec', path=spyt_driver_sample_script).exec_file
    write_spyt_driver_sample = yt_write_file(_name='Script -> YT', file=spyt_driver_sample, path=f'{deps_path}/{spyt_driver_sample_script}')

    # thread #3: deploy.tar -> unpack exec of driver -> write it on yt
    spyt_driver_saving = extract_op(_name='Extract results script', archive=package, out_type='exec', path=spyt_driver_saving_script).exec_file
    write_spyt_driver_saving = yt_write_file(_name='Script -> YT', file=spyt_driver_saving, path=f'{deps_path}/{spyt_driver_saving_script}')

    # thread #4: deploy.tar -> unpack exec of driver -> write it on yt
    spyt_driver_metrics = extract_op(_name='Extract metrics script', archive=package, out_type='exec', path=spyt_driver_metrics_script).exec_file
    write_spyt_driver_metrics = yt_write_file(_name='Script -> YT', file=spyt_driver_metrics, path=f'{deps_path}/{spyt_driver_metrics_script}')

    # operation: start spyt block on worker
    with vh.wait_for(write_src_zip, write_spyt_driver_sample):
        create_pool = spark_op(_name='Spark: create pool',
                               spyt_deps_dir=deps_path,
                               spyt_driver_filename=spyt_driver_sample_script,
                               spyt_driver_args=[
                                   f'--results_path {results_path}',
                                   f'--features_path {features_path}',
                                   f'--temporary_calcs_path {temporary_calcs_path}'
                               ])

    last_model_path = run_small_job_op(_name='Last model path', input=package, max_ram=128, yt_token='robot-clanalytics-yt',
                                       script='scripts/3_get_last_model_path.py').output
    catb = get_bin_from_yt(_name='model.bin path', ttl=60, _dynamic_options=last_model_path, path='-').output

    # get MR tables and files
    with vh.wait_for(create_pool):
        get_mr_pool = get_mr_table(_name='prediction_pool path', table=f'{temporary_calcs_path}/predict_pool').outTable
        get_mr_pool_cd = get_mr_table(_name='pool.cd path', table=f'{temporary_calcs_path}/pool.cd').outTable
        get_mr_output = get_mr_table(_name='result path', table=f'{temporary_calcs_path}/result').outTable

    # operation catboost applying
    catb_forecast = catb_apply(
        _name='Make prediction',
        _options={'prediction-type': 'RawFormulaVal', 'output-columns': ['SampleId', 'Label', 'RawFormulaVal']},
        _inputs={'pool': get_mr_pool, 'cd': get_mr_pool_cd, 'model.bin': catb, 'output_yt_table': get_mr_output}
    )

    with vh.wait_for(catb_forecast, write_spyt_driver_saving):
        save_res = spark_op(_name='Spark: save results',
                            spyt_deps_dir=deps_path,
                            spyt_driver_filename=spyt_driver_saving_script,
                            spyt_driver_args=[
                                f'--results_path {results_path}',
                                f'--temporary_calcs_path {temporary_calcs_path}'
                            ])

    with vh.wait_for(save_res, write_spyt_driver_metrics):
        calc_metrics = spark_op(_name='Spark: calc metrics',
                                spyt_deps_dir=deps_path,
                                spyt_driver_filename=spyt_driver_metrics_script,
                                spyt_driver_args=[
                                    f'--features_path {features_path}',
                                    f'--target_path {target_path}',
                                    f'--results_path {results_path}',
                                    f'--report_path {report_path}',
                                ])

    metrics_types = ['gen', 'onb', 'csm']
    with vh.wait_for(calc_metrics):
        outs = {}
        for mt in metrics_types:
            metrics_path = path_join(report_path, f'predict_{mt}_metrics')
            output = run_small_job_op(_name=f'Get {mt}-metrics', input=package,
                                      max_ram=128, yt_token='robot-clanalytics-yt',
                                      script='scripts/3_get_predict_metrics.py',
                                      script_args=[f'--metrics_path {metrics_path}']).output
            outs.update({mt: output})

    for mt, output in outs.items():
        pulsar_add_instance_op(
            _name=f'Add pulsar [{mt}]',
            _options={
                'model_name': f'Cons_predictor-predict-{mt}',
                'dataset_name': 'Last_available_7_days',
                'pulsar_token': 'robot-clanalytics-pulsar'
            },
            _inputs={'metrics': output}
        )

    workflow_id = 'e4a77cd3-d0b4-460f-9dd9-9e06fcb2d640' if is_prod else 'a4151731-335d-4779-8601-7d088b26e792'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='consumption_predictor_v2',
           quota='coud-analytics',
           nirvana_cached_external_data_reuse_policy='reuse_if_not_modified_strict',
           label=f'CatBoost predict prod ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
