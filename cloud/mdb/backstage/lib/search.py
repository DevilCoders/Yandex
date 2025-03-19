import six
import threading

from django.conf import settings
import django.core.exceptions as dce

import cloud.mdb.backstage.lib.apps as apps


FQDN_SEARCH_MODELS = []


if apps.CMS.is_enabled:
    import cloud.mdb.backstage.apps.cms.models as cms_models

if apps.META.is_enabled:
    import cloud.mdb.backstage.apps.meta.models as meta_models
    FQDN_SEARCH_MODELS.append(meta_models.Host)

if apps.KATAN.is_enabled:
    import cloud.mdb.backstage.apps.katan.models as katan_models
    FQDN_SEARCH_MODELS.append(katan_models.Host)

if apps.DEPLOY.is_enabled:
    import cloud.mdb.backstage.apps.deploy.models as deploy_models
    FQDN_SEARCH_MODELS.append(deploy_models.Master)
    FQDN_SEARCH_MODELS.append(deploy_models.Minion)

if apps.DBM.is_enabled:
    import cloud.mdb.backstage.apps.dbm.models as dbm_models
    FQDN_SEARCH_MODELS.append(dbm_models.Dom0Host)
    FQDN_SEARCH_MODELS.append(dbm_models.Container)


def get_object(model, filters, results):
    try:
        obj = model.objects.get(**filters)
        results.append(obj)
    except dce.ObjectDoesNotExist:
        pass


def run_fetcher(model, filters, results):
    fetcher = threading.Thread(
        target=get_object,
        args=(
            model,
            filters,
            results,
        )
    )
    fetcher.start()
    return fetcher


def search_by_fqdn(fqdns, results):
    if isinstance(fqdns, six.string_types):
        fqdns = [fqdns]

    if settings.INSTALLATION.is_compute():
        if settings.INSTALLATION.name == 'prod':
            for fqdn in fqdns:
                if fqdn.endswith('.db.yandex.net'):
                    fqdns.append(fqdn.replace('db.yandex.net', 'mdb.yandexcloud.net'))
        elif settings.INSTALLATION.name == 'preprod':
            for fqdn in fqdns:
                if fqdn.endswith('.db.yandex.net'):
                    fqdns.append(fqdn.replace('db.yandex.net', 'cloud-preprod.yandex.net'))

    fetchers = []
    for model in FQDN_SEARCH_MODELS:
        for fqdn in fqdns:
            fetchers.append(run_fetcher(model, {'fqdn': fqdn}, results))

    for fetcher in fetchers:
        fetcher.join()

    return results


def search_by_fqdn_links(fqdn):
    return [obj.self_ext_link for obj in search_by_fqdn(fqdn, results=[])]


def get_cms_last_decision(fqdn):
    if not apps.CMS.is_enabled or not apps.DBM.is_enabled:
        return None

    container = dbm_models.Container.objects\
        .select_related('dom0host')\
        .filter(fqdn=fqdn)\
        .first()
    if container:
        request = cms_models.Request.objects\
            .filter(fqdns=[container.dom0host.fqdn])\
            .select_related('decision')\
            .order_by('-created_at')\
            .first()
        if request:
            return request.decision


def search(q):
    q = q.strip()
    results = []
    fetchers = []

    if not q.isnumeric():
        if ':' in q:
            names = q.split(':')
        else:
            names = q

        fqdn_fetcher = threading.Thread(
            target=search_by_fqdn,
            args=(
                names,
                results,
            )
        )
        fqdn_fetcher.start()
        fetchers.append(fqdn_fetcher)
        if apps.META.is_enabled:
            fetchers.append(run_fetcher(meta_models.WorkerTask, {'task_id': q}, results))
            fetchers.append(run_fetcher(meta_models.Cluster, {'pk': q}, results))
            fetchers.append(run_fetcher(meta_models.Subcluster, {'pk': q}, results))
        if apps.CMS.is_enabled:
            fetchers.append(run_fetcher(cms_models.InstanceOperation, {'operation_id': q}, results))

    else:
        if apps.DEPLOY.is_enabled:
            fetchers.append(run_fetcher(deploy_models.Shipment, {'shipment_id': q}, results))
            fetchers.append(run_fetcher(deploy_models.JobResult, {'job_result_id': q}, results))
            fetchers.append(run_fetcher(deploy_models.JobResult, {'ext_job_id': q}, results))
        if apps.CMS.is_enabled:
            fetchers.append(run_fetcher(cms_models.Decision, {'pk': q}, results))

    for fetcher in fetchers:
        fetcher.join()

    return results
