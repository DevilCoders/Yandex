from .base_block import BaseBlock
from .processor_parameters import ProcessorType


NIRVANA_GET_FML_POOL_BY_ID = 'NIRVANA_GET_FML_POOL_BY_ID'
NIRVANA_GET_MR_TABLE_BY_FML_POOL = 'NIRVANA_GET_MR_TABLE_BY_FML_POOL'
NIRVANA_DOWNLOAD_POOL_PART_FROM_FML = 'NIRVANA_DOWNLOAD_POOL_PART_FROM_FML'
NIRVANA_COLLECT_POOL_ON_FML = 'NIRVANA_COLLECT_POOL_ON_FML'
NIRVANA_EVAL_FEATURE_ON_FML = 'NIRVANA_EVAL_FEATURE_ON_FML'
NIRVANA_UPLOAD_MR_POOL_TO_FML = 'NIRVANA_UPLOAD_MR_POOL_TO_FML'
NIRVANA_UPLOAD_POOL_TO_FML = 'NIRVANA_UPLOAD_POOL_TO_FML'


NIRVANA_GUIDS = {
    NIRVANA_GET_FML_POOL_BY_ID: '2e13cf82-58ac-4bf4-8611-7dd1739fafc7',
    NIRVANA_GET_MR_TABLE_BY_FML_POOL: '5dad5d8e-2682-11e6-a28b-5268111f66a3',
    NIRVANA_DOWNLOAD_POOL_PART_FROM_FML: 'e76d3e5e-02c0-11e5-9686-156cde17d4e4',
    NIRVANA_COLLECT_POOL_ON_FML: 'e733a7d8-62e7-11e6-80c1-9388f328ee33',
    NIRVANA_EVAL_FEATURE_ON_FML: '53e4b661-c038-43ff-bb68-4c48d6784ee2',
    NIRVANA_UPLOAD_MR_POOL_TO_FML: '47dbd02e-b7bb-11e6-bb4a-2958403a6036',
    NIRVANA_UPLOAD_POOL_TO_FML: 'dfd8837e-080e-11e7-a161-5b4753cafa85',
}


class FMLPoolTable:
    directory = 'directory'
    features = 'features'
    learn = 'learn'
    test = 'test'
    queries = 'queries'
    ratings = 'ratings'
    query_timestamps = 'query_timestamps'
    factor_slices = 'factor_slices'


class GzipPolicy:
    as_is = 'AS_IS'
    ungzip = 'UNGZIP'


class MRRuntimeType:
    yamr = 'YAMR'
    yt = 'YT'


class SearchType:
    web = 'WEB'
    img = 'IMG'
    video = 'VIDEO'
    music = 'MUSIC'
    news = 'NEWS'
    peoplesearch = 'PEOPLESEARCH'
    other = 'OTHER'
    selection_rank = 'SELECTION_RANK'
    personalization = 'PERSONALIZATION'
    snippets = 'SNIPPETS'
    export_rank = 'EXPORT_RANK'
    mascot = 'MASCOT'
    blender = 'BLENDER'
    recommendations = 'RECOMMENDATIONS'
    click_addition_web = 'CLICK_ADDITION_WEB'
    ppo = 'PPO'
    mobile_features = 'MOBILE_FEATURES'
    search_advertising = 'SEARCH_ADVERTISING'


class SplitLearnTestMode:
    no = 'NONE'
    kosher_ids = 'KOSHER_IDS'
    md5_hash = 'MD5_HASH'
    random_queries = 'RANDOM_QUERIES'
    random_part = 'RANDOM_PART'


class TargetStorageType:
    auto = 'AUTO'
    local = 'LOCAL'
    cluster = 'CLUSTER'


class EvalFeatureType:
    gpu = 'GPU'
    cpu = 'CPU'


class EvalFeatureTarget:
    mse = 'MSE'
    binary = 'BINARY_CLASS'
    pair = 'PAIR_LOGIT'


class EvalFeatureTest:
    all = 'WHOLE_RANGE'
    each_add = 'SEQ_ADD'
    each_and_all = 'SEQ_ADD_WHOLE'
    each_remove = 'SEQ_REMOVE'


class EvalFeatureFoldSize:
    exact_count = 'EXACT_COUNT'
    decimal_fraction = 'DECIMAL_FRACTION'
    simple_fraction = 'SIMPLE_FRACTION'


class NotifyChannelType:
    email = 'EMAIL'
    sms = 'SMS'

    # FIXME. Some operations require
    # list of values, others - just one value.
    # It's a mess.
    #
    none = 'NONE'
    email_and_sms = 'EMAIL_AND_SMS'


class NotifyEventType:
    completed = 'COMPLETED'
    failed = 'FAILED'


class GetFMLPoolById(BaseBlock):
    guid = NIRVANA_GUIDS[NIRVANA_GET_FML_POOL_BY_ID]
    name = 'Get FML Pool by Id'
    parameters = ['id']
    input_names = []
    output_names = ['outputPool']
    processor_type = ProcessorType.FML


class GetMRTableByFMLPool(BaseBlock):
    guid = NIRVANA_GUIDS[NIRVANA_GET_MR_TABLE_BY_FML_POOL]
    name = 'Get MR Table by FML Pool'
    parameters = ['table', 'preferred-mr-runtime-type', 'fail-on-runtime-mismatch']
    input_names = ['pool']
    output_names = ['table', 'directory']
    processor_type = ProcessorType.FML


class DownloadPoolPartFromFML(BaseBlock):
    guid = NIRVANA_GUIDS[NIRVANA_DOWNLOAD_POOL_PART_FROM_FML]
    name = 'Download Pool Part from FML'
    parameters = ['file-name', 'gzip-policy']
    input_names = ['fmlPool']
    output_names = ['part']
    processor_type = ProcessorType.FML


class CollectPoolOnFML(BaseBlock):
    guid = NIRVANA_GUIDS[NIRVANA_COLLECT_POOL_ON_FML]
    name = 'Collect Pool on FML'
    parameters = [
        'title',
        'comment',
        'tags',
        'search-type',
        'upper-host',
        'shard-tags',
        'custom-idx-ops-options',
        'robot-url-in-pool',
        'shard-map-source-beta',
        'beta-name',
        'unimplemented-factors-kept-in-pool',
        'is-mapreduce-pool',
        'mapreduce-cluster',
        'mapreduce-user',
        'is-direct-mapreduce-pool',
        'mapreduce-table',
        'split-mode',
        'kosher-pool-group-id',
        'test-percentage',
        'seed',
        'channels',
        'events',
        'cc-users',
        'timestamp',
    ]
    input_names = ['queries', 'ratings', 'wizardings', 'idx_ops']
    output_names = ['pool']
    processor_type = ProcessorType.FML


class UploadPoolToFML(BaseBlock):
    guid = NIRVANA_GUIDS[NIRVANA_UPLOAD_POOL_TO_FML]
    parameters = [
        'title',
        'comment',
        'tags',
        'category',
        'search-type',
        'split-mode',
        'kosher-pool-group-id',
        'test-percentage',
        'seed',
        'seed-source',
        'channels',
        'events',
        'cc-users',
        'timestamp'
    ]
    input_names = ['features', 'queries', 'factor_slices', 'poolFiles', 'parentPool']
    output_names = ['fmlPool']
    processor_type = ProcessorType.FML
    name_aliases = {'fmlPool': 'pool'}


class UploadMRPoolToFML(BaseBlock):
    guid = NIRVANA_GUIDS[NIRVANA_UPLOAD_MR_POOL_TO_FML]
    parameters = [
        'title',
        'comment',
        'tags',
        'search-type',
        'is-direct-mapreduce-pool',
        'preferred-target-storage',
        'split-mode',
        'kosher-pool-group-id',
        'test-percentage',
        'seed',
        'transmission-mode',
        'notification-events',
        'cc-users',
        'timestamp'
    ]
    input_names = ['table']
    output_names = ['pool']
    processor_type = ProcessorType.FML


class EvalFeatureOnFML(BaseBlock):
    guid = NIRVANA_GUIDS[NIRVANA_EVAL_FEATURE_ON_FML]
    name = 'Eval feature on FML'
    parameters = [
        'description',
        'type',
        'target',
        'how-to-test',
        'tested-factor',
        'test-factors-expression',
        'zero-factors-expression',
        'ignored-factor',
        'replaced-factor',
        'iterations',
        'regularization',
        'folds-count',
        'fold-size-def',
        'fold-size',
        'channels',
        'events',
        'cc-users',
        'custom-options',
    ]
    input_names = ['pool', 'tested_factors', 'ignored_factors', 'mx_binary']
    output_names = ['results']
    processor_type = ProcessorType.FML
