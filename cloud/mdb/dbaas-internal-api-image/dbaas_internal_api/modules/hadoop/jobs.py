# -*- coding: utf-8 -*-
"""
DBaaS Internal API Hadoop cluster jobs methods
"""
import datetime
import logging
import time
from base64 import b64decode, b64encode
from typing import Any, Dict, List, Optional

import marshmallow
from dateutil.parser import parse as date_parser
from flask import request, g, current_app
from flask.views import MethodView
from webargs.flaskparser import use_kwargs

from ...apis import API, marshal
from ...apis.config_auth import check_auth
from ...apis.schemas import fields
from ...core.exceptions import (
    DbaasClientError,
    DbaasNotImplementedError,
    DbaasForbidden,
    HadoopJobNotExistsError,
    HadoopJobLogNotFoundError,
)
from ...core.id_generators import gen_id
from ...utils import metadb
from ...utils.dataproc_joblog.api import DataprocJobLogError
from ...utils.dataproc_joblog.client import (
    DataprocJobLogAccessDenied,
    DataprocJobLogNoSuchBucket,
    DataprocJobLogNoSuchKey,
    DataprocJobLogNotFound,
)
from ...utils.filters_parser import Operator, parse
from ...utils.logging_read_service import (
    get_logging_read_service,
    LogGroupNotFoundError,
    LogGroupPermissionDeniedError,
    GrpcResponse,
)
from ...utils.logging_service import get_logging_service
from ...utils.operation_creator import create_unmanaged_operation
from ...utils.pagination import Column, supports_pagination
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE, MAX_CLOUD_LOGGING_PAGE_SIZE, FLUENTBIT_LOG_SEND_LAG
from .metadata import CreateJobMetadata
from .pillar import get_cluster_pillar, HadoopPillar
from .schemas import HadoopListJobsResponseSchema
from .traits import HadoopOperations, HadoopTasks, JobStatus
from .validation import validate_job
from .utils import get_joblog_client


def prepare_for_serialization(job: Dict):
    """
    Fill job's attributes
    """
    job['status'] = JobStatus.to_enum(job['status'])
    for key, value in job.get('job_spec', {}).items():
        job[key] = value


def trim_job_whitespaces(job_spec):
    job_type = next(iter(job_spec.keys()))
    spec = job_spec[job_type]
    for key, val in spec.items():
        if type(val) == str:
            spec[key] = val.strip()
        elif type(val) == list:
            spec[key] = [el.strip() if type(el) == str else el for el in val]
        else:
            continue
    return job_spec


def set_deploy_mode(pillar, job_spec):
    """
    Set spark.submit.deployMode=client for spark and pyspark jobs
    on lightweight clusters
    """
    if not pillar.is_lightweight():
        return job_spec
    for job_type in ['spark_job', 'pyspark_job']:
        job = job_spec.get(job_type)
        if job:
            job_spec[job_type]['properties']['spark.submit.deployMode'] = job['properties'].get(
                'spark.submit.deployMode', 'client'
            )
    return job_spec


@register_request_handler(MY_CLUSTER_TYPE, Resource.HADOOP_JOB, DbaasOperation.CREATE)
def create_job_handler(cluster: Dict, name: str, version: str, _schema, **job_spec):  # pylint: disable=unused-argument
    """
    Add Hadoop job handler
    """
    pillar = get_cluster_pillar(cluster)
    # Set default job name if not set
    if not name:
        name = "job-" + str(int(time.time()))
    job_id = gen_id('hadoop_job_id')
    validate_job(job_spec, pillar)
    job_spec = trim_job_whitespaces(job_spec)
    job_spec = set_deploy_mode(pillar, job_spec)

    metadb.add_hadoop_job(cluster['cid'], job_id, name, job_spec)
    return create_unmanaged_operation(
        task_type=HadoopTasks.job_create,
        operation_type=HadoopOperations.job_create,
        metadata=CreateJobMetadata(job_id),
        cid=cluster['cid'],
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.HADOOP_JOB, DbaasOperation.CANCEL)
def cancel_job_handler(cluster: Dict, job_id: str, **_):
    """
    Hadoop job cancel handler
    """
    job = metadb.get_hadoop_job(job_id)
    if not job:
        raise HadoopJobNotExistsError(job_id)
    prev_status = job['status']
    new_status = 'CANCELLING'
    if JobStatus.is_terminal(prev_status):
        raise DbaasClientError(f'Cannot cancel terminated job with status `{prev_status}`')
    cluster_id = cluster['cid']
    operation_id = metadb.get_hadoop_job_task(cluster_id=cluster_id, job_id=job_id)
    operation = metadb.get_operation_by_id(operation_id=operation_id)
    if prev_status == new_status:
        return operation
    metadb.update_hadoop_job_status(job_id, new_status, application_info=None)
    return operation


@register_request_handler(MY_CLUSTER_TYPE, Resource.HADOOP_JOB, DbaasOperation.INFO)
def get_job_handler(_: Dict, job_id: str, **__) -> Dict:
    """
    Hadoop job info handler
    """
    job = metadb.get_hadoop_job(job_id)
    if not job:
        raise HadoopJobNotExistsError(job_id)
    prepare_for_serialization(job)
    return job


def parse_filters(filters):
    """
    Gets statuses from filters and validates
    """
    if not filters:
        return None
    if len(filters) > 1:
        raise DbaasNotImplementedError('Unsupported filter. Only simple filter')
    filter_obj = filters[0]
    if filter_obj.attribute != 'status':
        raise DbaasNotImplementedError('Unsupported filter field "{}"'.format(filter_obj.attribute))

    statuses_str = []
    if filter_obj.operator in (Operator.equals, Operator.not_equals):
        statuses_str = [filter_obj.value]
    elif filter_obj.operator in (Operator.in_, Operator.not_in):
        statuses_str = filter_obj.value
    else:
        raise DbaasNotImplementedError('Unsupported filter operator. Must be equals or IN')

    statuses = []
    for status in statuses_str:
        status = str(status).upper()
        if JobStatus.has_member(status):
            statuses.append(status)
        elif status == "ACTIVE":
            statuses.extend(JobStatus.active_statuses())
        elif status == "ALL":
            statuses.extend(JobStatus.all_statuses())
        else:
            raise DbaasNotImplementedError(f'Unsupported status "{status}"')

    # If it's negation operator we need all statuses except given
    if filter_obj.operator in (Operator.not_in, Operator.not_equals):
        statuses = list(set(JobStatus.all_statuses()) - set(statuses))

    return statuses


@register_request_handler(MY_CLUSTER_TYPE, Resource.HADOOP_JOB, DbaasOperation.LIST)
@supports_pagination(
    items_field='jobs',
    columns=(Column(field='create_ts', field_type=date_parser), Column(field='job_id', field_type=str)),
)
def get_job_list_handler(
    cluster: Dict, page_token_create_ts=None, page_token_job_id=None, limit=None, **kwargs
) -> List[Dict]:
    """
    Hadoop job info handler
    """
    statuses = parse_filters(kwargs.get('filters', []))
    jobs = metadb.get_hadoop_jobs_by_cluster(
        cluster_id=cluster['cid'],
        statuses=statuses,
        page_token_create_ts=page_token_create_ts,
        page_token_job_id=page_token_job_id,
        limit=limit,
    )
    for job in jobs:
        prepare_for_serialization(job)
    return jobs


@API.resource('/mdb/hadoop/1.0/jobs')
class HadoopJobs(MethodView):
    """
    Jobs for all clusters. Private-api only method
    """

    @use_kwargs(
        {
            'access_id': marshmallow.fields.UUID(required=True, location='headers', load_from='Access-Id'),
            'access_secret': fields.Str(required=True, location='headers', load_from='Access-Secret'),
            'cluster_id': fields.Str(),
            'filters': fields.Str(),
            'page_size': fields.UInt(load_from='pageSize'),
            'page_token': fields.Str(load_from='pageToken'),
        }
    )
    @marshal.with_schema(HadoopListJobsResponseSchema)
    @supports_pagination(
        items_field='jobs',
        columns=(Column(field='create_ts', field_type=date_parser), Column(field='job_id', field_type=str)),
    )
    def get(
        self,
        access_id: str,
        access_secret: str,
        cluster_id=None,
        filters=None,
        page_token_create_ts=None,
        page_token_job_id=None,
        limit=None,
    ) -> List[Dict]:
        """
        Get Hadoop service jobs list. Private-api only method
        """
        check_auth(access_id, access_secret, 'dataproc-api')
        if filters:
            filters = parse(request.values.get('filters'))
        statuses = parse_filters(filters)
        if cluster_id:
            jobs = metadb.get_hadoop_jobs_by_cluster(
                cluster_id=cluster_id,
                statuses=statuses,
                page_token_create_ts=page_token_create_ts,
                page_token_job_id=page_token_job_id,
                limit=limit,
            )
        else:
            jobs = metadb.get_hadoop_jobs_by_service(
                statuses=statuses,
                page_token_create_ts=page_token_create_ts,
                page_token_job_id=page_token_job_id,
                limit=limit,
            )
        for job in jobs:
            prepare_for_serialization(job)
        return jobs


@API.resource('/mdb/hadoop/1.0/clusters/<string:cluster_id>/jobs/<string:job_id>:updateStatus')
class HadoopJob(MethodView):
    """
    Private-api only method for job status modification
    """

    @use_kwargs(
        {
            'access_id': marshmallow.fields.UUID(required=True, location='headers', load_from='Access-Id'),
            'access_secret': fields.Str(required=True, location='headers', load_from='Access-Secret'),
        }
    )
    def patch(self, cluster_id: str, job_id: str, **_):
        """
        Update Hadoop job status
        """
        new_status = request.json.get('status')
        application_info = request.json.get('application_info')
        if not new_status:
            raise DbaasClientError('You need to specify new status')
        if not JobStatus.has_member(new_status):
            raise DbaasClientError(f'Unknown status `{new_status}`')
        job = metadb.get_hadoop_job(job_id)
        prev_status = job['status']
        if prev_status == new_status:
            return {"clusterId": cluster_id, "jobId": job_id, "status": new_status}
        if JobStatus.is_terminal(prev_status):
            raise DbaasClientError(f'Can not change status from `{prev_status}`')
        if prev_status == "CANCELLING" and not JobStatus.is_terminal(new_status):
            raise DbaasClientError(f'Can not change status to `{new_status}` from `{prev_status}`')
        with metadb.commit_on_success():
            metadb.update_hadoop_job_status(job_id, new_status, application_info)
            if JobStatus.is_terminal(new_status):
                task_id = metadb.get_hadoop_job_task(cluster_id=cluster_id, job_id=job_id)
                result = JobStatus.is_success(new_status)
                metadb.finish_unmanaged_task(task_id=task_id, result=result)
        return {"clusterId": cluster_id, "jobId": job_id, "status": new_status}


def encode_page_token(page_token: int) -> str:
    return str(b64encode(str(page_token).encode('utf-8')), 'ascii')


def decode_page_token(page_token: Any) -> int:
    try:
        return int(b64decode(page_token.encode('utf-8')).decode('utf-8'))
    except Exception:
        raise DbaasClientError('Invalid pagination token')


@register_request_handler(MY_CLUSTER_TYPE, Resource.HADOOP_JOB, DbaasOperation.GET_HADOOP_JOB_LOG)
def get_job_log_handler(cluster: Dict, job_id: str, **kwargs) -> Dict:
    """
    Hadoop job log handler
    """

    job = metadb.get_hadoop_job(job_id)
    page_size = kwargs.get('page_size')
    page_token = kwargs.get('page_token')
    if not job:
        raise HadoopJobNotExistsError(job_id)
    if job['status'] in ('PROVISIONING', 'PENDING'):
        # Job not started, so return empty response with nextPageToken
        return {'content': '', 'nextPageToken': encode_page_token(0)}
    pillar = get_cluster_pillar(cluster)

    if pillar.logs_in_object_storage:
        return get_job_log_from_object_storage(job, pillar, cluster, page_size, page_token)
    else:
        return get_job_log_from_cloud_logging(job, pillar, cluster, page_size, page_token)


def get_job_log_from_object_storage(job: dict, pillar: HadoopPillar, cluster: dict, page_size, page_token):
    if not page_size:
        page_size = 2**20
    if page_token:
        page_token = decode_page_token(page_token)
    else:
        page_token = 0

    try:
        client = get_joblog_client(pillar)
        content, next_page_token = client.get_content(cluster['cid'], job['job_id'], offset=page_token, limit=page_size)
    except DataprocJobLogNotFound:
        raise HadoopJobLogNotFoundError(job['job_id'], pillar.user_s3_bucket)
    except DataprocJobLogAccessDenied:
        raise DbaasForbidden(
            f'service account {pillar.service_account_id} doesn\'t have access ' f'to bucket {pillar.user_s3_bucket}'
        )
    except DataprocJobLogNoSuchBucket:
        raise DbaasClientError(f'No such bucket {pillar.user_s3_bucket}')
    except DataprocJobLogNoSuchKey as e:
        raise DbaasClientError(e)
    except DataprocJobLogError:
        raise
    # If job is done and there's no more output then return empty next_page_token in order to signal client
    # that she has already scrolled to the very end of the job log
    if job['status'] in JobStatus.terminal_statuses() and page_token >= next_page_token:
        next_page_token = None  # type: ignore
    response: Dict[str, Any] = {'content': content}
    if next_page_token:
        response['next_page_token'] = encode_page_token(next_page_token)
    return response


def get_api_response(job: dict, logging_read_response: GrpcResponse) -> dict:
    response: Dict[str, Any] = {'content': '', 'next_page_token': logging_read_response.data.next_page_token}
    if logging_read_response.data.entries:
        lines = []
        for entry in logging_read_response.data.entries:
            lines.append(f'{entry.timestamp.ToJsonString()}\t{entry.message}')

        response['content'] = '{}\n'.format('\n'.join(lines))
        del lines
        if logging_read_response.data.entries[-1].message.startswith('Data Proc job finished.'):
            response['next_page_token'] = None  # type: ignore
            return response

    # If job is done and there's no more output then return empty next_page_token in order to signal client
    # that she has already scrolled to the very end of the job log
    elif job['status'] in JobStatus.terminal_statuses():
        # bad scenario when terminal string is not present in job output for some reason
        # do not stop log reading immediately after job in terminal status
        if job['end_ts']:
            time_since_job_terminated = datetime.datetime.now(datetime.timezone.utc) - job['end_ts']
            if time_since_job_terminated < FLUENTBIT_LOG_SEND_LAG:
                return response
            else:
                logger = logging.LoggerAdapter(
                    logging.getLogger(current_app.config['LOGCONFIG_BACKGROUND_LOGGER']),
                    extra={},
                )
                logger.error(f'time_since_job_terminated = {time_since_job_terminated}')

        response['next_page_token'] = None  # type: ignore
    return response


def get_job_log_from_cloud_logging(
    job: dict,
    pillar: HadoopPillar,
    cluster: dict,
    page_size: Optional[int],
    page_token: Optional[str],
) -> dict:
    if not page_size or page_size > MAX_CLOUD_LOGGING_PAGE_SIZE:
        page_size = MAX_CLOUD_LOGGING_PAGE_SIZE
    logging_read_service = get_logging_read_service()
    job_id = job['job_id']

    log_group_id = pillar.log_group_id
    try:
        if not log_group_id:
            log_group_service = get_logging_service()
            log_group_service.work_as_user_specified_account(pillar.service_account_id)
            log_group_id = log_group_service.get_default(folder_id=g.folder['folder_ext_id']).data.id

        # if "until" filter will not be set here, it will be set by logging service as ("since" + 1h)
        # however next_page_token will lead to next pages even if they are newer than ("since" + 1h)
        #
        # if "until" filter will be set here, pages will be returned backwards (newer to older)
        read_response = logging_read_service.read(
            log_group_id=log_group_id,
            filter_string=f'log_type:job_output AND job_id:{job_id}',
            resource_ids=[cluster['cid']],
            resource_types=['dataproc.cluster'],
            page_size=page_size,
            page_token=page_token,
            since=job['create_ts'],
        )
        return get_api_response(job, read_response)

    except (LogGroupNotFoundError, LogGroupPermissionDeniedError):
        raise DbaasClientError(
            f'Can not find log group {log_group_id}. '
            f'Or you do not have permission to read from log group {log_group_id}. '
            f'Make sure you have role "logging.reader" for the folder of this log group.'
        )
