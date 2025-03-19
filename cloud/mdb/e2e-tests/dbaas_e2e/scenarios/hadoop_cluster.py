"""
DBaaS E2E tests scenarios for hadoop
"""

import logging
import random
import string

from ..utils import geo_name, scenario


class CheckException(Exception):
    """
    Exception for case with empty jmx bean
    """


class IncorrectNodeState(Exception):
    """
    Unknown state of node
    """


class DataprocJobException(Exception):
    """
    Exception for cases with failed dataproc jobs
    """


class AutoscalingError(Exception):
    """
    General error with autoscaling subclusters
    """


def _filter_job(job: dict):
    """
    Return significant fields for failed dataproc job
    """
    d = dict()
    for key in ['id', 'name', 'status', 'startedAt', 'finishedAt']:
        v = job.get(key)
        if v is not None:
            d[key] = v
    return d


def get_nodes(hosts, role=None):
    """
    Return fqdns of nodes
    """
    if not role:
        return [x['name'] for x in hosts['hosts']]
    return [x['name'] for x in hosts['hosts'] if x['role'] == role]


def get_masternodes(hosts):
    """
    Return fqdn of masternodes
    """
    return get_nodes(hosts, role='MASTERNODE')


def get_datanodes(hosts):
    """
    Return fqdn of datanodes
    """
    return get_nodes(hosts, role='DATANODE')


def get_computenodes(hosts):
    """
    Return fqdn of computenodes
    """
    return get_nodes(hosts, role='COMPUTENODE')


def get_instance_group_fqdns(fqdns):
    """
    Return fqdn of instance group based computenodes
    """
    return [fqdn for fqdn in fqdns if '-g-' in fqdn]


def get_instance_group_computenodes(hosts):
    """
    Return fqdn of instance group based computenodes
    """
    return get_instance_group_fqdns(get_computenodes(hosts))


class HadoopBaseClusterCreate:
    """
    Scenario for hadoop creation
    """

    CLUSTER_TYPE = 'hadoop'
    SERVICE_LIST = ['HDFS', 'YARN', 'MAPREDUCE', 'SPARK', 'TEZ', 'HIVE']
    AUTOSCALING_SUBCLUSTER = False
    CHECK_AUTOSCALING_GROWTH = False
    IMAGE_VERSION = '2.0'
    WITH_DATANODES = True

    logger = logging

    @classmethod
    def get_options(cls, config):
        """
        Returns options for hadoop cluster creation
        """

        subcluster_spec = [
            {
                'name': 'main',
                'role': 'MASTERNODE',
                'resources': {
                    'resourcePresetId': config.hadoop_flavor,
                    'diskTypeId': config.disk_type,
                    'diskSize': 21474836480,
                },
                'subnetId': config.dualstack_subnet_id or config.subnet_id,
            },
        ]

        if cls.WITH_DATANODES:
            subcluster_spec.append(
                {
                    'name': 'data',
                    'role': 'DATANODE',
                    'resources': {
                        'resourcePresetId': config.hadoop_flavor,
                        'diskTypeId': config.disk_type,
                        'diskSize': 21474836480,
                    },
                    'hostsCount': 1,
                    'subnetId': config.dualstack_subnet_id or config.subnet_id,
                },
            )

        compute_subcluster_spec = {
            'name': 'compute',
            'role': 'COMPUTENODE',
            'resources': {
                'resourcePresetId': config.hadoop_flavor,
                'diskTypeId': config.disk_type,
                'diskSize': 21474836480,
            },
            'hostsCount': 1,
            'subnetId': config.dualstack_subnet_id or config.subnet_id,
        }

        if cls.AUTOSCALING_SUBCLUSTER:
            compute_subcluster_spec['autoscalingConfig'] = {
                'maxHostsCount': 2,
                'preemptible': True,
            }
        subcluster_spec.append(compute_subcluster_spec)

        return {
            'configSpec': {
                'versionId': cls.IMAGE_VERSION,
                'hadoop': {
                    'services': cls.SERVICE_LIST,
                    'sshPublicKeys': [config.ssh_public_key],
                    'properties': {
                        'yarn:yarn.log-aggregation-enable': 'false',
                    },
                },
                'subclustersSpec': subcluster_spec,
            },
            'bucket': config.s3_bucket,
            'serviceAccountId': config.sa_creds['service_account_id'],
            'zoneId': geo_name(config, 'myt'),
            'uiProxy': True,
            'logGroupId': getattr(config, 'hadoop_log_group_id', ''),
        }

    @staticmethod
    def get_pyspark_job(bucket: str):
        """
        Return definition of dataproc spark job
        """
        return {
            'name': 'pyspark e2e job',
            'pysparkJob': {
                'mainPythonFileUri': 's3a://dataproc-e2e/jobs/sources/pyspark-001/main.py',
                'pythonFileUris': ['s3a://dataproc-e2e/jobs/sources/pyspark-001/geonames.py'],
                'args': [
                    's3a://dataproc-e2e/jobs/sources/data/cities500.txt.bz2',
                    f's3a://{bucket}/jobs/results/' + '${JOB_ID}',
                ],
                'fileUris': ['s3a://dataproc-e2e/jobs/sources/data/config.json'],
                'archiveUris': ['s3a://dataproc-e2e/jobs/sources/data/country-codes.csv.zip'],
                'jarFileUris': [
                    's3a://dataproc-e2e/jobs/sources/java/dataproc-examples-1.0.jar',
                    's3a://dataproc-e2e/jobs/sources/java/commons-lang-2.6.jar',
                    's3a://dataproc-e2e/jobs/sources/java/opencsv-4.1.jar',
                    's3a://dataproc-e2e/jobs/sources/java/json-20190722.jar',
                    's3a://dataproc-e2e/jobs/sources/java/icu4j-61.1.jar',
                ],
                'properties': {
                    'spark.yarn.tags': 'e2e',
                },
            },
        }

    @staticmethod
    def get_spark_job(bucket: str):
        """
        Return definition of dataproc spark job
        """
        return {
            'name': 'spark e2e job',
            'sparkJob': {
                'mainJarFileUri': 's3a://dataproc-e2e/jobs/sources/java/dataproc-examples-1.0.jar',
                'mainClass': 'ru.yandex.cloud.dataproc.examples.PopulationSparkJob',
                'args': [
                    's3a://dataproc-e2e/jobs/sources/data/cities500.txt.bz2',
                    f's3a://{bucket}/jobs/results/' + '${JOB_ID}',
                ],
                'fileUris': ['s3a://dataproc-e2e/jobs/sources/data/config.json'],
                'archiveUris': ['s3a://dataproc-e2e/jobs/sources/data/country-codes.csv.zip'],
                'jarFileUris': [
                    's3a://dataproc-e2e/jobs/sources/java/icu4j-61.1.jar',
                    's3a://dataproc-e2e/jobs/sources/java/opencsv-4.1.jar',
                    's3a://dataproc-e2e/jobs/sources/java/json-20190722.jar',
                ],
                'properties': {
                    'spark.yarn.tags': 'e2e',
                },
            },
        }

    @staticmethod
    def get_spark_pi_job():
        """
        Return definition of dataproc spark job
        """
        return {
            'name': 'spark e2e pi calc',
            'sparkJob': {
                'mainJarFileUri': 'file:///usr/lib/spark/examples/jars/spark-examples.jar',
                'mainClass': 'org.apache.spark.examples.SparkPi',
                'args': [
                    '1000',
                ],
                'properties': {
                    'spark.yarn.tags': 'e2e',
                },
            },
        }

    @staticmethod
    def get_pyspark_pi_job():
        """
        Return definition of dataproc spark job
        """
        return {
            'name': 'pyspark e2e pi calc',
            'pysparkJob': {
                'mainPythonFileUri': 'file:///usr/lib/spark/examples/src/main/python/pi.py',
                'args': [
                    '1000',
                ],
                'properties': {
                    'spark.yarn.tags': 'e2e',
                },
            },
        }

    @staticmethod
    def get_hive_job():
        """
        Return definition of dataproc spark job
        """
        return {
            'name': 'hive e2e job',
            'hiveJob': {
                'scriptVariables': {
                    'CITIES_URI': 's3a://dataproc-e2e/jobs/sources/data/hive-cities/',
                    'COUNTRY_CODE': 'RU',
                },
                'queryFileUri': 's3a://dataproc-e2e/jobs/sources/hive-001/main.sql',
            },
        }

    @staticmethod
    def get_mapreduce_job(bucket: str):
        """
        Return definition of dataproc mapreduce job
        """
        output_dir = ''.join(random.choice(string.ascii_lowercase) for _ in range(20))  # nosec
        return {
            'name': 'mapreduce e2e job',
            'mapreduceJob': {
                'mainJarFileUri': 's3a://dataproc-e2e/jobs/sources/java/dataproc-examples-1.0.jar',
                'jarFileUris': [
                    's3a://dataproc-e2e/jobs/sources/java/opencsv-4.1.jar',
                    's3a://dataproc-e2e/jobs/sources/java/icu4j-61.1.jar',
                    's3a://dataproc-e2e/jobs/sources/java/commons-lang-2.6.jar',
                    's3a://dataproc-e2e/jobs/sources/java/json-20190722.jar',
                ],
                'fileUris': ['s3a://dataproc-e2e/jobs/sources/data/config.json'],
                'archiveUris': ['s3a://dataproc-e2e/jobs/sources/data/country-codes.csv.zip'],
                'args': [
                    'ru.yandex.cloud.dataproc.examples.PopulationMRJob',
                    's3a://dataproc-e2e/jobs/sources/data/cities500.txt.bz2',
                    f's3a://{bucket}/jobs/results/{output_dir}',
                ],
            },
        }

    @classmethod
    def get_job_arguments(cls, config):
        return [
            HadoopClusterCreate.get_spark_job(config.s3_bucket),
            HadoopClusterCreate.get_pyspark_job(config.s3_bucket),
            HadoopClusterCreate.get_mapreduce_job(config.s3_bucket),
            HadoopClusterCreate.get_hive_job(),
        ]

    @classmethod
    def check_dataproc_jobs(cls, config, api_client, cluster_id, logger=None, sequential_run=True):
        jobs_arguments = cls.get_job_arguments(config)
        operations_to_wait = []
        for job_arguments in jobs_arguments:
            operation = api_client.dataproc_job_create(
                cluster_id,
                job_arguments,
                wait=sequential_run,
            )
            job_id = operation['metadata']['jobId']
            operations_to_wait.append((operation['id'], job_id))

        if not sequential_run:
            for operation_id, job_id in operations_to_wait:
                api_client.wait_operation(operation_id, raise_on_failure=False, timeout=600)

        failed_jobs = []
        for operation_id, job_id in operations_to_wait:
            job_info = api_client.dataproc_job_info(cluster_id, job_id)
            job_status = job_info['status']
            if job_status != 'DONE':
                failed_jobs.append(_filter_job(job_info))
                logger.error(f'job {job_id} status = {job_status}')

        if failed_jobs:
            raise DataprocJobException(f'dataproc cluster {cluster_id} has failed jobs: {failed_jobs}')

    @classmethod
    def post_check(cls, config, hosts, api_client=None, cluster_id=None, **_):
        """
        Post-creation checks
        """

        # Init logger
        cls.logger = logging.getLogger(cls.__name__)

        instance_group_computenodes = []
        if cls.AUTOSCALING_SUBCLUSTER:
            instance_group_computenodes = get_instance_group_computenodes(
                api_client.cluster_hosts(
                    cluster_type=cls.CLUSTER_TYPE,
                    cluster_id=cluster_id,
                )
            )
            cls.logger.info(f'instance_group_computenodes = {instance_group_computenodes}')

        cls.check_dataproc_jobs(
            config,
            api_client,
            cluster_id,
            logger=cls.logger,
            sequential_run=cls.SEQUENTIAL_RUN,
        )

        if cls.AUTOSCALING_SUBCLUSTER and cls.CHECK_AUTOSCALING_GROWTH:
            new_instance_group_computenodes = get_instance_group_computenodes(
                api_client.cluster_hosts(
                    cluster_type=cls.CLUSTER_TYPE,
                    cluster_id=cluster_id,
                )
            )
            cls.logger.info(f'new_instance_group_computenodes = {new_instance_group_computenodes}')

            new_ig_nodes_have_been_added = len(new_instance_group_computenodes) > len(instance_group_computenodes)
            if not new_ig_nodes_have_been_added:
                raise AutoscalingError(f'Autoscaling subcluster of cluster {cluster_id} did not grow after being load')


@scenario
class HadoopClusterCreate(HadoopBaseClusterCreate):
    """
    Basic scenario for backward compatability
    """

    pass


@scenario
class Dataproc_2_0_ClusterCreate(HadoopBaseClusterCreate):
    """
    Scenario for dataproc-2-0 with full services
    """

    SERVICE_LIST = [
        'HDFS',
        'YARN',
        'MAPREDUCE',
        'SPARK',
        'TEZ',
        'HIVE',
        'HBASE',
        'OOZIE',
        'ZEPPELIN',
        'ZOOKEEPER',
        'LIVY',
    ]
    IMAGE_VERSION = '2.0'
    CLUSTER_NAME_SUFFIX = '_20'
    AUTOSCALING_SUBCLUSTER = True
    CHECK_AUTOSCALING_GROWTH = False
    SEQUENTIAL_RUN = False


@scenario
class DataprocLightweight_2_0_ClusterCreate(HadoopBaseClusterCreate):
    """
    Scenario for dataproc-2-0 without datanodes (w/o hbase and oozie)
    """

    SERVICE_LIST = [
        'YARN',
        'SPARK',
        'LIVY',
    ]
    IMAGE_VERSION = '2.0'
    CLUSTER_NAME_SUFFIX = '_20_lw'
    AUTOSCALING_SUBCLUSTER = True
    CHECK_AUTOSCALING_GROWTH = False
    WITH_DATANODES = False
    SEQUENTIAL_RUN = True

    @classmethod
    def get_job_arguments(cls, config):
        return [
            HadoopClusterCreate.get_spark_pi_job(),
            HadoopClusterCreate.get_pyspark_pi_job(),
        ]


@scenario
class DataprocLightweight_2_1_ClusterCreate(HadoopBaseClusterCreate):
    """
    Scenario for dataproc-2-1 without datanodes (w/o hbase and oozie)
    """

    SERVICE_LIST = [
        'YARN',
        'SPARK',
        'LIVY',
    ]
    IMAGE_VERSION = '2.1'
    CLUSTER_NAME_SUFFIX = '_21_lw'
    AUTOSCALING_SUBCLUSTER = True
    CHECK_AUTOSCALING_GROWTH = False
    WITH_DATANODES = False
    SEQUENTIAL_RUN = True

    @classmethod
    def get_job_arguments(cls, config):
        return [
            HadoopClusterCreate.get_spark_pi_job(),
            HadoopClusterCreate.get_pyspark_pi_job(),
        ]
