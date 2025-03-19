import vh
import os
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import extract_op, zip_op, spark_op, yt_write_file, get_mr_table, catb_train, catb_apply, run_small_job_op, pulsar_add_instance_op
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
    features_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_features"
    target_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_target"
    metrics_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/metrics"
    models_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/models_history"
    calibrators_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/calibrators_history"

    # scripts
    spyt_driver_script = 'scripts/2_make_prod_sample_for_learn.py'
    gen_metrics_script = 'scripts/2_gen_eval_metrics.py'
    onb_metrics_script = 'scripts/2_onb_calibr_and_eval_metrics.py'
    csm_metrics_script = 'scripts/2_csm_calibr_and_eval_metrics.py'
    csm_metrics_get_script = 'scripts/2_csm_get_last_metrics.py'

    # yt tables pathes
    prod_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/"
    samples_path = os.path.join(prod_path, 'last_train_samples')
    pool_train_path = os.path.join(samples_path, 'train')
    pool_val_path = os.path.join(samples_path, 'val')
    pool_train_val_path = os.path.join(samples_path, 'train_val')
    pool_test_path = os.path.join(samples_path, 'test')
    pool_oot_path = os.path.join(samples_path, 'oot')
    pool_cd_path = os.path.join(samples_path, 'pool.cd')

    # thread #1: deploy.tar -> unpack bin of src folder -> compress to zip -> write it on yt
    src = extract_op(_name='Extract deps', archive=package, out_type='binary', path='src').binary_file
    src_zip = zip_op(_name='deps to ".zip"', input=src).output
    write_src_zip = yt_write_file(_name='deps.zip -> YT', file=src_zip, path=f'{deps_path}/dependencies.zip')

    # thread #2: deploy.tar -> unpack exec of driver -> write it on yt
    spyt_driver = extract_op(_name='Extract driver script', archive=package, out_type='exec', path=spyt_driver_script).exec_file
    write_spyt_driver = yt_write_file(_name='Script -> YT', file=spyt_driver, path=f'{deps_path}/{spyt_driver_script}')

    # thread #3: deploy.tar -> unpack exec of driver -> write it on yt
    spyt_driver_csm = extract_op(_name='Extract CSM calibr', archive=package, out_type='exec', path=csm_metrics_script).exec_file
    write_spyt_driver_csm = yt_write_file(_name='Script -> YT', file=spyt_driver_csm, path=f'{deps_path}/{csm_metrics_script}')

    # operation: start spyt block on worker
    with vh.wait_for(write_src_zip, write_spyt_driver):
        spyt_samples = spark_op(_name='Make prod samples',
                                spyt_deps_dir=deps_path,
                                spyt_driver_filename=spyt_driver_script,
                                spyt_driver_args=[
                                    f'--features_path {features_path}',
                                    f'--target_path {target_path}',
                                    f'--samples_path {samples_path}'
                                ])

    with vh.wait_for(spyt_samples):
        get_mr_train_pool = get_mr_table(_name='train path', table=pool_train_path).outTable
        get_mr_val_pool = get_mr_table(_name='val path', table=pool_val_path).outTable
        get_mr_cd_pool = get_mr_table(_name='pool.cd path', table=pool_cd_path).outTable

    catb_model = catb_train(
        _name='Training model',
        _inputs={
            'learn': get_mr_train_pool,
            'test': get_mr_val_pool,
            'cd': get_mr_cd_pool
        },
        _options={
            'loss-function': 'MAE',
            'iterations': 1000,
            'learning-rate': 0.01,
            'seed': 0,
            'feature-border-type': 'MinEntropy',
            'depth': 10,
            'border-count': 1024,
            'use-best-model': True,
            'od_type': 'IncToDec',
            'od-pval': 0.01,
        },
    )

    catb_model_bin = catb_model.__getitem__('model.bin')

    with vh.wait_for(catb_model):
        saving_path_option = run_small_job_op(_name='Saving path', input=package, max_ram=128, script='scripts/2_make_model_path.py').output
    saving_model = yt_write_file(_name='Save model', file=catb_model_bin, _dynamic_options=saving_path_option, path='-')

    with vh.wait_for(catb_model):
        get_mr_train_val_pool = get_mr_table(_name='train_val path', table=pool_train_val_path).outTable
        get_mr_test_pool = get_mr_table(_name='test path', table=pool_test_path).outTable
        get_mr_oot_pool = get_mr_table(_name='oot path', table=pool_oot_path).outTable
        get_mr_train_val_output = get_mr_table(_name='train_val path', table=os.path.join(samples_path, 'result_train_val')).outTable
        get_mr_test_output = get_mr_table(_name='train_val path', table=os.path.join(samples_path, 'result_test')).outTable
        get_mr_oot_output = get_mr_table(_name='train_val path', table=os.path.join(samples_path, 'result_oot')).outTable

    train_val_apply = catb_apply(
        _name='Make prediction',
        _options={'prediction-type': 'RawFormulaVal', 'output-columns': ['SampleId', 'RawFormulaVal', 'Label']},
        _inputs={'pool': get_mr_train_val_pool, 'cd': get_mr_cd_pool, 'model.bin': catb_model_bin, 'output_yt_table': get_mr_train_val_output}
    )
    test_apply = catb_apply(
        _name='Make prediction',
        _options={'prediction-type': 'RawFormulaVal', 'output-columns': ['SampleId', 'RawFormulaVal', 'Label']},
        _inputs={'pool': get_mr_test_pool, 'cd': get_mr_cd_pool, 'model.bin': catb_model_bin, 'output_yt_table': get_mr_test_output}
    )
    oot_apply = catb_apply(
        _name='Make prediction',
        _options={'prediction-type': 'RawFormulaVal', 'output-columns': ['SampleId', 'RawFormulaVal', 'Label']},
        _inputs={'pool': get_mr_oot_pool, 'cd': get_mr_cd_pool, 'model.bin': catb_model_bin, 'output_yt_table': get_mr_oot_output}
    )

    with vh.wait_for(train_val_apply, test_apply, oot_apply, saving_model):
        metrics_gen = run_small_job_op(
            _name='Prepare gen metrics', input=package, script=gen_metrics_script, max_ram=4*1024,
            yt_token='robot-clanalytics-yt',
            script_args=[
                f'--metrics_path {metrics_path}/train_gen_metrics',
                f'--samples_path {samples_path}',
                f'--models_path {models_path}'
            ]
        ).output
        metrics_onb = run_small_job_op(
            _name='Prepare onb metrics', input=package, script=onb_metrics_script, max_ram=4*1024,
            yql_token='robot-clanalytics-yql', yt_token='robot-clanalytics-yt',
            script_args=[
                f'--metrics_path {metrics_path}/train_onb_metrics',
                f'--features_path {features_path}',
                f'--samples_path {samples_path}',
                f'--models_path {models_path}',
                f'--calibrators_path {calibrators_path}/onboarding'
            ]
        ).output

    with vh.wait_for(train_val_apply, test_apply, oot_apply, saving_model, write_src_zip, write_spyt_driver_csm):
        spyt_metrics = spark_op(_name='Load csm metrics',
                                spyt_deps_dir=deps_path,
                                spyt_driver_filename=csm_metrics_script,
                                spyt_driver_args=[
                                    f'--metrics_path {metrics_path}/train_csm_metrics',
                                    f'--features_path {features_path}',
                                    f'--samples_path {samples_path}',
                                    f'--models_path {models_path}',
                                    f'--calibrators_path {calibrators_path}/csm'
                                ])

    with vh.wait_for(spyt_metrics):
        metrics_csm = run_small_job_op(_name='Prepare csm metrics', input=package, script=csm_metrics_get_script, max_ram=512,
                                       yt_token='robot-clanalytics-yt', script_args=[f'--metrics_path {metrics_path}/train_csm_metrics']).output

    pulsar_add_instance_op(
        _name='Add pulsar [general]',
        _options={'model_name': 'Cons_predictor-train-gen', 'dataset_name': 'OOT', 'pulsar_token': 'robot-clanalytics-pulsar'},
        _inputs={'metrics': metrics_gen}
    )

    pulsar_add_instance_op(
        _name='Add pulsar [onboard]',
        _options={'model_name': 'Cons_predictor-train-onb', 'dataset_name': 'OOT', 'pulsar_token': 'robot-clanalytics-pulsar'},
        _inputs={'metrics': metrics_onb}
    )

    pulsar_add_instance_op(
        _name='Add pulsar [CSM]',
        _options={'model_name': 'Cons_predictor-train-csm', 'dataset_name': 'OOT', 'pulsar_token': 'robot-clanalytics-pulsar'},
        _inputs={'metrics': metrics_csm}
    )

    workflow_id = '5be69752-2865-4959-8c0e-bf3335dff5db' if is_prod else 'a4151731-335d-4779-8601-7d088b26e792'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='consumption_predictor_v2',
           quota='coud-analytics',
           nirvana_cached_external_data_reuse_policy='reuse_if_not_modified_strict',
           label=f'Collect features and target ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
