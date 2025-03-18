class ProcessorType(object):
    Job = 'job'
    JobMasterSlave = 'job_master_slave'
    Sandbox = 'sandbox'
    MR = 'MR'
    GetMRPath = 'get_mr_path'
    Hive = 'hive'
    Hitman = 'hitman'
    FML = 'FML'


ProcessorParameters = {
    ProcessorType.Job: [
        'job-command', 'job-binary-url', 'job-environments', 'job-variables', 'job-volume', 'ttl', 'max-ram',
        'cpu-cores', 'cpu-usage-per-core', 'retries-on-system-failure', 'retries-on-job-failure', 'ports',
        'gpu-type', 'gpu-count', 'gpu-max-ram', 'job-is-vanilla', 'debug-timeout', 'max-disk',
    ],
    ProcessorType.JobMasterSlave: [
        'slaves', 'ports', 'master-job-command', 'slave-job-command', 'master-job-binary-url', 'slave-job-binary-url',
        'master-job-environments', 'slave-job-environments', 'master-job-variables', 'slave-job-variables', 'ttl',
        'master-max-ram', 'slave-max-ram', 'master-cpu-cores', 'master-cpu-usage-per-core', 'slave-cpu-cores',
        'slave-cpu-usage-per-core', 'retries-on-system-failure', 'retries-on-job-failure', 'master-slave-mode',
        'job-is-vanilla', 'master-max-disk', 'slave-max-disk',
    ],
    ProcessorType.MR: [
        'job-command', 'job-binary-url', 'job-environments', 'job-variables', 'job-volume',
        'yt-token', 'mr-output-path', 'mr-account', 'yt-pool', 'yt-use-yamr-defaults',
        'ttl', 'max-ram', 'cpu-cores', 'cpu-usage-per-core', 'retries-on-system-failure', 'retries-on-job-failure',
        'debug-timeout', 'mr-default-cluster', 'max-disk',
    ],
    ProcessorType.Sandbox: [
        'sandbox_task_type', 'sandbox_requirements_disk', 'sandbox_requirements_ram',
    ],
    ProcessorType.GetMRPath: [],
    ProcessorType.Hitman: [],
    ProcessorType.Hive: [],
    ProcessorType.FML: [],
    None: []
}
