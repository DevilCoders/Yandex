from flask import abort
from app.quota import QuotaService
from app.quota.subject import Subject
from app.config import Config
from app import log
from app.quota.constants import CONVERTABLE_VALUES, SERVICES
from app.quota.utils.validators import validate_subject_id
from app.quota.utils.helpers import human_to_bytes
from app.restapi import dbrequest

# ToDo setup ENV globally from Config
ENV = 'prod'

ENDPOINTS = SERVICES


def _init_service(service: str, iam_token=None) -> QuotaService:
    """Return service object with methods."""
    endpoint = SERVICES[service]['endpoint'][ENV]
    quota_service = QuotaService(iam_token=iam_token or Config.OAUTH, env=ENV, endpoint=endpoint,
                                 service=service, ssl='static/allCAs.pem')

    return quota_service


def get_subject(service: str, subject_id: str, iam_token: str) -> Subject:
    """Get service quota metrics as subject and return."""
    validate_subject_id(subject_id)
    quota = _init_service(service, iam_token)
    subject = quota.get_metrics(subject_id)
    return subject


def update_concrete(service: str, subject_id: str, metric: str, value: [str, int]) -> None:
    """Update specified service quota metric."""
    validate_subject_id(subject_id)
    quota = _init_service(service)
    valid_value = human_to_bytes(value) if metric in CONVERTABLE_VALUES else int(value)

    try:
        quota.update_metric(subject_id, metric, valid_value)
        print(f'{metric} set to {value} - OK')
    except Exception as err:
        print(err)


def update_from_token(token, iam_token=None, subject=None):
    if len(token) != 6:
        abort(500, 'Token error')
    data = dbrequest(f"SELECT data from qfiles where name = '{token}'", response=True)
    data = data[0][0]
    cloudid = data.pop('cloudId')
    servicelist = data.keys()
    responce = {}
    for servicename in servicelist:
        service = _init_service(servicename, iam_token=iam_token)
        subject_id = validate_subject_id(subject or cloudid)
        metrics = data[servicename]
        ends = ('b', 'k', 'm', 'g', 't')
        for metric in metrics.keys():
            name = metric
            limit = metrics[metric]
            limit = human_to_bytes(limit) if name in CONVERTABLE_VALUES and str(limit).lower().endswith(ends) else int(limit)
            try:
                service.update_metric(subject_id, name, limit)
                responce[servicename] = 'ok'
            except Exception as err:
                log.error(err)
                responce[servicename] = err
    log.info(f'Quotas for {cloudid} upped from {token}')
    return responce
