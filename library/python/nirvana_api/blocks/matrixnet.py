from .base_block import BaseBlock
from .processor_parameters import ProcessorType


class LearningMethod(object):
    Pfound = ' -F'
    MSE = ' '
    RMSEWithoutQueryAverage = ' -q'
    LambdaRank = ' -u'
    YetiRank = ' -U'
    MultiClassification = ' -m'
    PairPreference = ' -P'
    Classification = ' -c'
    LLG = '--llg'
    LLMAX = '--llmax'
    CrossEntropy = '--cross-entropy'
    ExperimentalDCG = ' -V'


class CompositeLearningMethod(object):
    Pfound = 'F'
    MSE = 'MSE'
    QMSE = 'QMSE'
    LambdaRank = 'FFlat'
    YetiRank = 'YetiRank'
    MultiClassification = 'MultiClassification'
    PairLogit = 'PairLogit'
    Logit = 'Logit'
    LLG = 'LLG'
    LLMAX = 'LLMAX'
    CrossEntropy = 'CrossEntropy'
    ExperimentalDCG = 'V'


class GPULearningMethod(object):
    Pfound = 'F'
    MSE = 'MSE'
    QMSE = 'QMSE'
    Logit = 'Logit'
    CrossEntropy = 'CrossEntropy'
    LambdaRank = 'LambdaRank'
    PairLogit = 'PairLogit'
    QueryCrossEntropy = 'QCrossEntropy'
    LLMAX = 'LLMax'
    CRR = 'CRR'
    NonDiagQMse = 'NonDiagQMse'
    MseAndQMse = 'MseAndQMse'
    PairwiseMse = 'PairwiseMse'


class GPUGridType(object):
    GreedyLogSum = 'GreedyLogSum'
    Median = 'Median'
    UniformAndQuantiles = 'UniformAndQuantiles'
    MinEntropy = 'MinEntropy'
    MaxLogSum = 'MaxLogSum'


_base_matrixnet_parameters = [
    'iterations', 'regularisation', 'discretisation', 'ignore', 'learning-method', 'classBorder', 'seed',
    'cross', 'takenfraction', 'maxcomplexity', 'args', 'W', 'alpha', 'yt-token',
]

_master_slave_parameters = ['split', 'slaveArgs', 'master-job-host-tags', 'slave-job-host-tags']


class Matrixnet(BaseBlock):
    parameters = _base_matrixnet_parameters
    input_names = [
        'learn', 'test',
        'mxCpu', 'borders',
        'pairs', 'testPairs',
        'learnBaseLine', 'testBaseLine',
        'learnQueryWeights', 'testQueryWeights',
        'config',
        'externalProgress',
    ]
    output_names = [
        'matrixnet.info',
        'matrixnet.bin',
        'matrixnet.fstr',
        'matrixnet.fpair',
        'matrixnet.prog',
        'matrixnet.trees',
        'all.tar.gz',
        'matrixnet.log',
        'stdout.log',
        'out.borders',
        'learn.tsv.matrixnet',
        'test.tsv.matrixnet',
    ]
    name_aliases = {
        'mxCpu': ['binary'],
        'classBorder': ['class_border'],
        'takenfraction': ['taken_fraction'],
        'regularisation': ['regularization'],
        'discretisation': ['discretization'],
    }


class SingleNodeMatrixnet(Matrixnet):
    guid = '5dbe513b-9693-41cc-8295-f2a3421a02e7'
    name = 'Train matrixnet on CPU (single node) [official from ML team]'
    processor_type = ProcessorType.Job


class MasterSlaveMatrixnet(Matrixnet):
    guid = '0ccb855f-7423-42aa-918d-1460d0106d95'
    name = 'Train matrixnet on CPU [official from ML team]'
    parameters = _base_matrixnet_parameters + _master_slave_parameters
    processor_type = ProcessorType.JobMasterSlave


class MasterSlaveMatrixnetAutoResources(Matrixnet):
    guid = '7d8a82fe-eb82-41be-b44e-edf0da98b234'
    name = 'Train matrixnet on CPU [official from ML team]'
    parameters = _base_matrixnet_parameters + ['job-is-vanilla', 'ttl', 'fixed-slaves', 'master-job-host-tags', 'slave-job-host-tags', 'clos', 'job-scheduler']


class GPUMatrixnet(BaseBlock):
    guid = '908164d2-9d37-4802-aaff-5e124ecebc1f'
    name = 'Train matrixnet on GPU [official from ML team]'
    parameters = [
        'gpu-type', 'gpu-count', 'gpu-max-ram',
        'iterations', 'regularization', 'discretization', 'ignore', 'target', 'class_border', 'grid',
        'taken_fraction', 'qwise_selection', 'seed', 'W', 'cross', 'yt_token', 'binarize_on_load',
        'other_options', 'time_split', 'baseline_column', 'features_per_iteration_fraction',
    ]
    processor_type = ProcessorType.Job
    output_names = ['matrixnet.info', 'matrixnet.bin', 'matrixnet.part.bin', 'all.tar.gz', 'matrixnet.grid']
    input_names = [
        'features', 'test',
        'mn_cuda', 'features_pairs', 'test_pairs',
        'grid_file', 'matrixnet.part.bin', 'queryTimestamps', 'config',
    ]
    name_aliases = {
        'mn_cuda': ['binary'],
        'features': ['learn'],
        'test_pairs': ['testPairs'],
        'features_pairs': ['pairs'],

        'target': ['learning_method'],
        'class_border': ['classBorder'],
        'other_options': ['args'],
        'taken_fraction': ['takenfraction'],
        'regularization': ['regularisation'],
        'discretization': ['discretisation'],
    }


class GPUMatrixnetAutoResources(BaseBlock):
    guid = '07a75424-234a-463e-83f4-d58eb4d0a105'
    name = 'Train matrixnet on GPU with auto resources estimation [official from ML team]'
    parameters = [
        'gpu-count', 'gpu-type',
        'iterations', 'regularisation', 'discretisation', 'ignore', 'learning-method', 'classBorder', 'grid',
        'takenfraction', 'qwise_selection', 'seed', 'W', 'cross', 'yt-token',
        'args', 'time_split', 'baseline_column', 'features_per_iteration_fraction',
        'mr-account', 'retries-on-system-failure', 'retries-on-job-failure', 'yt-pool'
    ]
    processor_type = ProcessorType.Job
    output_names = ['matrixnet.info', 'matrixnet.bin', 'matrixnet.prog', 'all.tar.gz', 'matrixnet.grid']
    input_names = [
        'learn', 'test',
        'pairs', 'testPairs',
        'mn_cuda',
        'borders',
        'externalProgress',
        'queryTimestamps',
        'config',
    ]
    name_aliases = {
        'learn': ['features'],
        'mn_cuda': ['binary'],

        'args': ['other_options'],
        'classBorder': ['class_border'],
        'takenfraction': ['taken_fraction'],
        'regularisation': ['regularization'],
        'discretisation': ['discretization'],
        'learning-method': ['target'],
    }


class GPUMatrixnetAutoResourcesWithYTPool(GPUMatrixnetAutoResources):
    guid = '2cbebd52-d4b0-4419-9354-e2efaa054026'
    name = 'Train matrixnet on GPU with auto resources estimation and with yt pool passing to FML stats'
    parameters = GPUMatrixnetAutoResources.parameters + ['yt_pool']


class CatboostWithMatrixnetInterface(GPUMatrixnetAutoResources):
    guid = '91f3a8e3-5570-42d1-a67e-bac77baf010f'
    name = 'Train catboost with matrixnet interface'
    cb_params = [
        'yt_pool',
        'catboost_args',
        'cb-gpu-type',
        'loss-function-param',
        'inner-options-override',
        'random-strength',
        'restrict-gpu-type'
    ]
    parameters = [param for param in GPUMatrixnetAutoResources.parameters if param not in {'yt-pool'}] + cb_params
    output_names = [out for out in GPUMatrixnetAutoResources.output_names if out not in {'matrixnet.grid'}] + ['eval_result', 'matrixnet.fstr', 'training_log.json']
    name_aliases = {
        'learn': ['features'],
        'mn_cuda': ['binary'],
        'args': ['other_options'],
        'classBorder': ['class_border'],
        'takenfraction': ['taken_fraction'],
        'regularisation': ['regularization'],
        'discretisation': ['discretization'],
        'learning-method': ['target'],
        'catboost_args': ['catboost_other_options'],
    }
