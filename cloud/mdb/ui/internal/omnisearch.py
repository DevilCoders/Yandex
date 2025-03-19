from django.conf import settings
from django.db.models import Q


from cloud.mdb.ui.internal.deploy.models import JobResult, Master, Minion, Shipment

if settings.ENABLED_APPS.katan:
    from cloud.mdb.ui.internal.katan.models import Host as KatanHost
from cloud.mdb.ui.internal.meta.models import Cluster, Host, Subcluster, WorkerQueue

if settings.ENABLED_APPS.cms:
    from cloud.mdb.ui.internal.cms.models import Request
from cloud.mdb.ui.internal.mdbui.installation import InstallationType


if settings.ENABLED_APPS.dbm:
    from cloud.mdb.ui.internal.dbm.models import Dom0Host, Container


def search_by_fqdn(fqdn):
    result = []
    meta_host = Host.objects.filter(fqdn=fqdn).first()
    if meta_host:
        result.append(meta_host.link_ext)
    deploy_master = Master.objects.filter(fqdn=fqdn).first()
    if deploy_master:
        result.append(deploy_master.link_ext)
    deploy_minion = Minion.objects.filter(fqdn=fqdn).first()
    if deploy_minion:
        result.append(deploy_minion.link_ext)
    if settings.ENABLED_APPS.katan:
        katan_host = KatanHost.objects.filter(fqdn=fqdn).first()
        if katan_host:
            result.append(katan_host.link_ext)

    if settings.INSTALLATION == InstallationType.compute_prod:
        if fqdn.endswith('.mdb.yandexcloud.net'):
            result.extend(search_by_fqdn(fqdn.replace('mdb.yandexcloud.net', 'db.yandex.net')))
        elif fqdn.endswith('.db.yandex.net'):
            result.extend(search_by_fqdn(fqdn.replace('db.yandex.net', 'mdb.yandexcloud.net')))
    elif settings.INSTALLATION == InstallationType.compute_preprod:
        if fqdn.endswith('.cloud-preprod.yandex.net'):
            result.extend(search_by_fqdn(fqdn.replace('cloud-preprod.yandex.net', 'db.yandex.net')))
        elif fqdn.endswith('.db.yandex.net'):
            result.extend(search_by_fqdn(fqdn.replace('db.yandex.net', 'cloud-preprod.yandex.net')))
    if settings.ENABLED_APPS.dbm:
        container = Container.objects.filter(fqdn=fqdn).first()
        if container:
            result.append(container.link_ext)
        dom0_host = Dom0Host.objects.filter(fqdn=fqdn).first()
        if dom0_host:
            result.append(dom0_host.link_ext)
    return result


def search(q):
    q = q.strip()
    if not q.isnumeric():
        result = search_by_fqdn(q)
        worker_task = WorkerQueue.objects.filter(task_id=q).first()
        if worker_task:
            result.append(worker_task.link_ext)
        meta_cluster = Cluster.objects.filter(cid=q).first()
        if meta_cluster:
            result.append(meta_cluster.link_ext)
        meta_subcluster = Subcluster.objects.filter(subcid=q).first()
        if meta_subcluster:
            result.append(meta_subcluster.link_ext)
    else:
        result = []
        shipment = Shipment.objects.filter(shipment_id=q).first()
        if shipment:
            result.append(shipment.link_ext)
        job_result = JobResult.objects.filter(Q(job_result_id=q) | Q(ext_job_id=q)).first()
        if job_result:
            result.append(job_result.link_ext)
    return result


def get_cms_last_decision(fqdn):
    if settings.ENABLED_APPS.cms:
        return None
    container = Container.objects.filter(fqdn=fqdn).first()
    if container:
        request = Request.objects.filter(fqdns=[container.dom0.fqdn]).order_by('-created_at').first()
        if request:
            return request.decision
