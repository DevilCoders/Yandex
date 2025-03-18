from .base_block import BaseBlock
from .matrixnet import Matrixnet, MasterSlaveMatrixnet, _base_matrixnet_parameters
from .processor_parameters import ProcessorType


class CatboostLearningMethod(object):
    class Regression(object):
        RMSE = 'RMSE'
        MAE = 'MAE'
        Quantile = 'Quantile'
        LogLinQuantile = 'LogLinQuantile'
        Poisson = 'Poisson'
        MAPE = 'MAPE'

    class Classification(object):
        Logloss = 'Logloss'
        CrossEntropy = 'CrossEntropy'

    class Multiclassification(object):
        MultiClass = 'MultiClass'
        MultiClassOneVsAll = 'MultiClassOneVsAll'

    class Ranking(object):
        YetiRank = 'YetiRank'
        YetiRankPairwise = 'YetiRankPairwise'
        QueryRMSE = 'QueryRMSE'
        QuerySoftMax = 'QuerySoftMax'
        QueryCrossEntropy = 'QueryCrossEntropy'

    class Pairwise(object):
        PairLogit = 'PairLogit'
        PairLogitPairwise = 'PairLogitPairwise'


class CatboostWithMatrxinetInterfaceLearningMethod(object):
    Pfound = ' -F'
    MSE = ''
    YetiRank = ' -U'
    MultiClassification = ' -m'
    PairPreference = ' -P'
    Classification = ' -c'
    LLMAX = ' --llmax'
    QSoftMax = '--qsoftmax'
    CrossEntropy = ' --cross-entropy'


class TrainCatboost(BaseBlock):
    guid = '2f4fdddf-26f5-4e29-94e6-b079e5d393cf'
    name = 'Train CatBoost'
    parameters = [
        'gpu-type', 'slaves', 'yt-token', 'yt-scheme', 'yt_pool',
        'debug-timeout',
        'args', 'final-ctr-computation-mode', 'create-tensorboard',
        'loss-function', 'loss-function-param',
        'iterations', 'learning-rate', 'rsm', 'seed', 'has-time', 'max-ctr-complexity', 'od-pval', 'ctr-border-count',
        'one-hot-max-size', 'simple-ctr', 'combinations-ctr', 'per-feature-ctr', 'feature-border-type', 'counter-calc-method',
        'ignored-features', 'use-best-model', 'l2-leaf-reg', 'border-count', 'fold-len-multiplier',
        'fold-permutation-block', 'depth', 'X', 'bagging-temperature', 'random-strength', 'approx-on-full-history',
        'store-all-simple-ctr', 'ctr-leaf-count-limit',
        'has-header', 'delimiter',
        'calc-fstr', 'prediction-type',
        'arcadia_revision_cb_binary', 'arcadia_revision_cube', 'opempi-catboost-resource-id-str', 'cube_id_str',
        'subsample', 'bootstrap_type', 'cpu-guarantee', 'sampling-unit', 'leaf-estimation-method', 'leaf-estimation-iterations',
        'od_type', 'od-pval', 'eval-metric', 'custom-metric', 'cv-fold-index', 'cv-fold-count', 'fstr-type', 'target-border',
        'boost-from-average',
        'inner-options-override',
    ]
    input_names = [
        'learn', 'test', 'cd',
        'catboost_binary',
        'pairs', 'testPairs',
        'params_file',
        'snapshot_file',
    ]
    output_names = ['eval_result',
                    'model.bin',
                    'fstr',
                    'learn_error.log',
                    'test_error.log',
                    'training_log.json',
                    'plots.html',
                    'snapshot_file',
                    ]
    name_aliases = {
        'catboost_binary': ['binary'],
        'loss-function': ['learning_method'],
    }
    processor_type = ProcessorType.JobMasterSlave


class CatboostWithMatrixnetInterface(Matrixnet):
    guid = 'defffc2d-20db-4ca0-ae01-6d293eac6383'
    name = 'Train CatBoost with Matrixnet interface'
    parameters = _base_matrixnet_parameters + [
        'job-is-vanilla', 'catboost_args', 'cb-gpu-type', 'yt-scheme', 'restrict-gpu-type'
    ]
    input_names = MasterSlaveMatrixnet.input_names + ['catboost_binary']
    output_names = ['eval_result',
                    'model.bin',
                    'fstr',
                    'learn_error.log',
                    'test_error.log',
                    'snapshot_file',
                    'matrixnet.info',
                    'all.tar.gz',
                    'error.html'
                    ]
    processor_type = ProcessorType.Job


class ApplyCatboost(BaseBlock):
    guid = 'cf5fc001-ba54-468c-bc70-96ee44f10704'
    name = 'Apply catboost model'
    parameters = [
        'ttl',
        'max-ram',
        'cpu-cores',
        'debug-timeout',
        'tree-count-limit',
        'prediction-type',
        'args',
    ]
    input_names = [
        'pool',
        'model.bin',
        'cd',
        'catboost_binary',
    ]
    output_names = ['result']
    processor_type = ProcessorType.Job


class EvalMetricsCatboost(BaseBlock):
    guid = 'd7e74a5d-bbe2-4a5d-abb0-954bcd1e792b'
    name = 'Eval metrics Catboost'
    parameters = ['cpu-cores', 'metrics', 'ntree-start', 'ntree-end', 'eval-period', 'args', 'arcadia_revision', 'create-tensorboard']
    input_names = ['pool', 'model.bin', 'cd', 'catboost_external_binary']
    output_names = ['result', 'plots.html', 'tensorboard_url']
    processor_type = ProcessorType.Job


class CatboostModelAnalysis(BaseBlock):
    guid = 'e5ccfda1-b4ae-4084-96b5-5a7184ab03e1'
    name = 'Catboost Model Analysis'
    parameters = ['threads', 'analyse-mode', 'has-header', 'delimiter', 'arcadia-revision', 'max-ram']
    input_names = ['pool', 'cd', 'model.bin', 'catboost_external_binary']
    output_names = ['result']

    class AnalyseMode:
        fstr = 'fstr'
        ostr = 'ostr'


class CatboostOstr(CatboostModelAnalysis):
    parameters = CatboostModelAnalysis.parameters + ['update-method', 'top']
    analyse_mode = 'ostr'
    input_names = CatboostModelAnalysis.input_names + ['test', 'train_pairs']

    class UpdateMethod:
        SinglePoint = 'SinglePoint'
        TopKLeaves = 'TopKLeaves'
        AllPoints = 'AllPoints'


class CatboostFstr(CatboostModelAnalysis):
    parameters = CatboostModelAnalysis.parameters + ['fstr-type', 'class-names']
    analyse_mode = 'fstr'

    class FstrType:
        FeatureImportance = 'FeatureImportance'
        InternalFeatureImportance = 'InternalFeatureImportance'
        Interaction = 'Interaction'
        InternalInteraction = 'InternalInteraction'
        Doc = 'Doc'


class CatboostPoolQuantization(BaseBlock):
    guid = '17f9cae8-69c9-459f-acca-a7513fca0ce5'
    name = 'Catboost Pool Quantization'

    parameters = [
        'border-count',
        'class-names'
        'delimiter',
        'grid-type',
        'has-header',
        'ignored-features',
        'nan-mode',
    ]
    input_names = ['pool', 'cd', 'quantization_schema', 'borders']
    output_names = ['quantized_pool', 'quantization_schema']


class CatboostCalculateQuantizationSchema(BaseBlock):
    guid = 'b0f5be2c-59cf-4d9d-9f12-35b9f8ecbc37'
    name = 'Catboost Calculate Quantization Schema'

    parameters = [
        'border-count',
        'class-names'
        'delimiter',
        'grid-type',
        'has-header',
        'ignored-features',
        'nan-mode',
        'quantization-schema-format',
        'random-seed',
        'row-sampling-rate',
    ]
    input_names = ['pool', 'cd']
    output_names = ['quantization_schema']


class CatboostSumModels(BaseBlock):
    guid = '21d81aea-0fad-4866-a72d-9f8357dc9eda'
    name = 'Catboost Sum Models'

    parameters = [
        'sum_type',
        'weights'
    ]
    input_names = ['models']
    output_names = ['model.bin']
