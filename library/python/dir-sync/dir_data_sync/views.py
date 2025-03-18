# coding: utf-8
import json
import logging
from datetime import timedelta

from django.conf import settings
from django.db.models import Q
from django.http import JsonResponse, HttpResponse
from django.utils import timezone
from django.views.decorators.http import require_POST, require_GET
from rest_framework.decorators import permission_classes
from rest_framework.generics import GenericAPIView

from .logic import delete_org_data
from .models import ChangeEvent, Organization
from .permissions import IsAuthenticated
from .tasks.sync_data import ProcessChangeEventForOrgTask
from .utils import import_dir_organization

logger = logging.getLogger(__name__)


@permission_classes((IsAuthenticated,))
@require_POST
def process_data_change_request(request):
    """
    Принять и обработать запрос-уведомление от Директории об изменении в структуре организации.

    Возвращает статус-код 200 в случае успешной обработки запроса.
    """

    data = json.loads(request.body)

    logger.debug('Got request from Directory: data="%s"', repr(data))

    dir_org_id = data.get('org_id')
    if not dir_org_id:
        raise AttributeError('org_id not found')

    req_id = request.META.get('HTTP_X_REQUEST_ID')

    if (data.get('event') in ('service_enabled', 'service_disabled')) and \
            (data['object']['slug'] != settings.DIRSYNC_SERVICE_SLUG):
        pass
    else:
        ProcessChangeEventForOrgTask().delay(dir_org_id=str(dir_org_id), req_id_from_dir=req_id)

    return JsonResponse({'status': 'ok'}, status=200)


@require_GET
def get_org_data_sync_statistics(request):
    """
    Вернуть статистические данные о количестве организаций, данные которых были синхронизированы с Директорией
    в течение последних 12 часов и данные которых не были синхронизированы за тот же промежуток времени.
    """
    timeout = timezone.now() - timedelta(hours=12)

    stale_orgs_count = ChangeEvent.objects.filter(Q(last_pull_at__lt=timeout) | Q(last_pull_at__isnull=True)).count()
    synced_orgs_count = ChangeEvent.objects.filter(last_pull_at__gte=timeout).count()

    result = [["directory_sync_stale_organisations_ahhh", stale_orgs_count],
              ["directory_sync_synced_organisations_ahhh", synced_orgs_count]]

    return JsonResponse(result, status=200, safe=False)


class OrganizationView(GenericAPIView):

    def put(self, request, dir_org_id, *args, **kwargs):
        org = self.dir_client.get_organization(dir_org_id)
        if Organization.objects.filter(dir_id=dir_org_id).exists():
            delete_org_data(dir_org_id)
        import_dir_organization(org)
        return HttpResponse('ok', status=200)

    def delete(self, request, dir_org_id, *args, **kwargs):
        delete_org_data(dir_org_id)
        return HttpResponse('ok', status=200)
