import asyncio

from django.core.management import BaseCommand
from s_keeper.models import Support_Unit, Queue_Filter, ComponentsDict
from s_keeper.serializers import Support_UnitSerializer, Queue_FilterSerializer
from s_keeper.externals import download_persons
from uuid import uuid4


class Command(BaseCommand):

    def handle(self, *args, **options):
        SLA_FAILED = '''"sla": {
                        "slaSettings": [1775],
                        "violationStatus": ["FAIL_CONDITIONS_VIOLATED"]
                        }'''
        SLA_WARNING = '''"sla": {
                        "slaSettings": [1775],
                        "violationStatus": ["WARN_CONDITIONS_VIOLATED"]
                        }
                        '''
        SLA_FAILED_QUERY = 'Filter: 426514'
        QUEUE = '"queue": "CLOUDSUPPORT"'

        Support_Unit.objects.all().delete()
        Queue_Filter.objects.all().delete()

        persons = asyncio.run(download_persons())

        for person in persons:
            data = {
                "u_id": uuid4(),
                "login": person['person']['login'],
                "is_absent": False,
                "is_fivetwo": True,
                "is_duty": False
            }
            new_unit = Support_Unit(**data)
            new_unit_srlzr = Support_UnitSerializer(data=data)
            new_unit_srlzr.is_valid()
            new_unit.save(new_unit_srlzr.validated_data)

        filters = [
            {"q_id": uuid4(),
             "name": 'Протухшие',
             "ts_open": '{ %s, %s }' % (QUEUE, SLA_FAILED),
             'query_type': 'filter',
             'crew_danger_limit': 1,
             'crew_warning_limit': 2,
             'ts_danger_limit': 10,
             'ts_warning_limit': 7},
            {'q_id': uuid4(),
             'name': 'Почти протухшие',
             'ts_open': '{ %s, %s}' % (QUEUE, SLA_WARNING),
             'query_type': 'filter',
             'crew_danger_limit': 1,
             'crew_warning_limit': 2,
             'ts_danger_limit': 10,
             'ts_warning_limit': 7},
            {'q_id': uuid4(),
             'name': 'Critical Managed',
             'ts_open': 'Queue: CLOUDSUPPORT Components: '
                        '!квоты Pay: free, standard, unknown Priority: Критичный, Блокер '
                        '("Account Manager": !empty() and "Account Manager": !"Дмитрий Троегубов" '
                        'and "Account Manager": !"Мария Стрижак" )  Filter: 428079 ',
                        # 'Status: open, inProgress',
             'query_type': 'query',
             'crew_danger_limit': 1,
             'crew_warning_limit': 2,
             'ts_danger_limit': 10,
             'ts_warning_limit': 7},
            {'q_id': uuid4(),
             'name': 'Critical',
             'ts_open': 'Queue: CLOUDSUPPORT Components: '
                        '!квоты Components: !opstasks Pay: free, standard, business, unknown Priority: Критичный, '
                        'Блокер   Filter: 428079',  # Status: open, inProgress ',
             'query_type': 'query',
             'crew_danger_limit': 1,
             'crew_warning_limit': 2,
             'ts_danger_limit': 10,
             'ts_warning_limit': 7},
        #     {'q_id': uuid4(),
        #      'name': 'Sales Escalated',
        #      'ts_open': 'Resolution: empty() Queue: CLOUDSUPPORT Status: open, inProgress Tags: '
        #                 'sales_escalated Filter: 428079',
        #      'ts_in_progress': 'Resolution: empty() Queue: CLOUDSUPPORT Status: inProgress Tags: '
        #                 'sales_escalated Filter: 428079',
        #      'ts_sla_failed': 'Resolution: empty() Queue: CLOUDSUPPORT Status: open, inProgress Tags: '
        #                 'sales_escalated %s' % SLA_FAILED_QUERY,
        #      'query_type': 'query',
        #      'crew_danger_limit': 1,
        #      'crew_warning_limit': 2,
        #      'ts_danger_limit': 10,
        #      'ts_warning_limit': 7},
        #     {'q_id': uuid4(),
        #      'name': 'Newbies approve',
        #      'ts_open': 'Queue: CLOUDSUPPORT Tags: на_аппрув Filter: 428079',
        #      'ts_in_progress': 'Queue: CLOUDSUPPORT Tags: на_аппрув status: inProgress Filter: 428079',
        #      'ts_sla_failed': 'Queue: CLOUDSUPPORT Tags: на_аппрув %s' % SLA_FAILED_QUERY,
        #      'query_type': 'query',
        #      'crew_danger_limit': 1,
        #      'crew_warning_limit': 2,
        #      'ts_danger_limit': 10,
        #      'ts_warning_limit': 7},
        #     {'q_id': uuid4(),
        #      'name': 'Billing/OPS',
        #      'ts_open': 'Resolution: empty() Queue: CLOUDSUPPORT Status: open, inProgress Components: '
        #                 'billing, billing.antifraud, billing.cards, billing.cheki, billing.delete, billing.refund, '
        #                 'billing.trial, opstasks Filter: 428079',
        #      'ts_in_progress': 'Resolution: empty() Queue: CLOUDSUPPORT Status: inProgress '
        #                        'Components: billing, billing.antifraud, billing.cards, billing.cheki, billing.delete, '
        #                        'billing.refund, billing.trial, opstasks Filter: 428079',
        #      'ts_sla_failed': 'Queue: CLOUDSUPPORT Status: open, inProgress Components: '
        #                 'billing, billing.antifraud, billing.cards, billing.cheki, billing.delete, billing.refund, '
        #                 'billing.trial, opstasks %s' % SLA_FAILED_QUERY,
        #      'query_type': 'query',
        #      'crew_danger_limit': 1,
        #      'crew_warning_limit': 2,
        #      'ts_danger_limit': 10,
        #      'ts_warning_limit': 7},
        #     {'q_id': uuid4(),
        #      'name': 'Cards',
        #      'ts_open': 'Queue: CLOUDSUPPORT Status: open, inProgress Components: billing, '
        #                 'billing.antifraud, billing.cards, billing.cheki, billing.delete, billing.refund, '
        #                 'billing.trial, opstasks Filter: 428079"',
        #      'ts_in_progress': 'Queue: CLOUDSUPPORT Status:  inProgress Components: billing, '
        #                 'billing.antifraud, billing.cards, billing.cheki, billing.delete, billing.refund, '
        #                 'billing.trial, opstasks Filter: 428079',
        #      'ts_sla_failed': 'Queue: CLOUDSUPPORT Status: open, inProgress Components: billing, '
        #                 'billing.antifraud, billing.cards, billing.cheki, billing.delete, billing.refund, '
        #                 'billing.trial, opstasks %s' % SLA_FAILED_QUERY,
        #      'query_type': 'query',
        #      'crew_danger_limit': 1,
        #      'crew_warning_limit': 2,
        #      'ts_danger_limit': 10,
        #      'ts_warning_limit': 7},
        #     {'q_id': uuid4(),
        #      'name': 'Cards/Reg',
        #      'ts_open': 'Queue: CLOUDSUPPORT Status: open, inProgress Components: billing.cards, trial, '
        #                 'billing.antifraud, grant Filter: 428079',
        #      'ts_in_progress': 'Queue: CLOUDSUPPORT Status: inProgress Components: billing.cards, trial, '
        #                 'billing.antifraud, grant Filter: 428079',
        #      'ts_sla_failed': 'Queue: CLOUDSUPPORT Status: open, inProgress Components: billing.cards, trial, '
        #                 'billing.antifraud, grant %s' % SLA_FAILED_QUERY,
        #      'query_type': 'query',
        #      'crew_danger_limit': 1,
        #      'crew_warning_limit': 2,
        #      'ts_danger_limit': 10,
        #      'ts_warning_limit': 7},
        #     {'q_id': uuid4(),
        #      'name': 'ALB K8S',
        #      'ts_open': 'queue: CLOUDSUPPORT Components: k8s, "app balancer" Status: Открыт, inProgress  '
        #                 'Filter: 428079 pay: !premium',
        #      'ts_in_progress': 'queue: CLOUDSUPPORT Components: k8s, "app balancer" Status: inProgress  '
        #                 'Filter: 428079 pay: !premium',
        #      'ts_sla_failed': 'queue: CLOUDSUPPORT Components: k8s, 51713 Status: Открыт, inProgress  '
        #                 '%s pay: !premium' % SLA_FAILED_QUERY,
        #      'query_type': 'query',
        #      'crew_danger_limit': 1,
        #      'crew_warning_limit': 2,
        #      'ts_danger_limit': 10,
        #      'ts_warning_limit': 7},
        #     {'q_id': uuid4(),
        #      'name': 'Reopen',
        #      'ts_open': 'Queue: CLOUDSUPPORT Status: open Tags: reopen Filter: 428079',
        #      'ts_in_progress': 'Queue: CLOUDSUPPORT Status: open Tags: reopen Filter: 428079',
        #      'ts_sla_failed': 'Queue: CLOUDSUPPORT Status: open Tags: reopen %s' % SLA_FAILED_QUERY,
        #      'query_type': 'query',
        #      'crew_danger_limit': 1,
        #      'crew_warning_limit': 2,
        #      'ts_danger_limit': 10,
        #      'ts_warning_limit': 7},
        #     {'q_id': uuid4(),
        #      'name': 'Quotas/returned',
        #      'ts_open': 'Queue: CLOUDSUPPORT Status: open, InProgress Components: вернули  Filter: 428079',
        #      'ts_in_progress': 'Queue: CLOUDSUPPORT Status: open Components: вернули  Filter: 428079',
        #      'ts_sla_failed': 'Queue: CLOUDSUPPORT Status: open, InProgress Components: вернули  %s' % SLA_FAILED_QUERY,
        #      'query_type': 'query',
        #      'crew_danger_limit': 1,
        #      'crew_warning_limit': 2,
        #      'ts_danger_limit': 10,
        #      'ts_warning_limit': 7},
        #     {'q_id': uuid4(),
        #      'name': 'Premium',
        #      'ts_open': 'Queue: CLOUDSUPPORT Status: open, InProgress pay: premium Filter: 428079',
        #      'ts_in_progress': 'Queue: CLOUDSUPPORT Status: InProgress pay: premium Filter: 428079',
        #      'ts_sla_failed': 'Queue: CLOUDSUPPORT Status: open, InProgress pay: premium %s' % SLA_FAILED_QUERY,
        #      'query_type': 'query',
        #      'crew_danger_limit': 1,
        #      'crew_warning_limit': 2,
        #      'ts_danger_limit': 10,
        #      'ts_warning_limit': 7},
        #     {'q_id': uuid4(),
        #      'name': 'Business',
        #      'ts_open': 'Queue: CLOUDSUPPORT Status: open, InProgress pay: business Filter: 428079',
        #      'ts_in_progress': 'Queue: CLOUDSUPPORT Status: InProgress pay: business Filter: 428079',
        #      'ts_sla_failed': 'Queue: CLOUDSUPPORT Status: open, InProgress pay: business %s' % SLA_FAILED_QUERY,
        #      'query_type': 'query',
        #      'crew_danger_limit': 1,
        #      'crew_warning_limit': 2,
        #      'ts_danger_limit': 10,
        #      'ts_warning_limit': 7},
        #     {'q_id': uuid4(),
        #      'name': 'Standard',
        #      'ts_open': 'Queue: CLOUDSUPPORT Status: open, InProgress pay: standard Filter: 428079',
        #      'ts_in_progress': 'Queue: CLOUDSUPPORT Status: InProgress pay: standard Filter: 428079',
        #      'ts_sla_failed': 'Queue: CLOUDSUPPORT Status: open, InProgress pay: standard %s' % SLA_FAILED_QUERY,
        #      'query_type': 'query',
        #      'crew_danger_limit': 1,
        #      'crew_warning_limit': 2,
        #      'ts_danger_limit': 10,
        #      'ts_warning_limit': 7},
        #     {'q_id': uuid4(),
        #      'name': 'Free',
        #      'ts_open': 'Queue: CLOUDSUPPORT Status: open, InProgress pay: free Filter: 428079',
        #      'ts_in_progress': 'Queue: CLOUDSUPPORT Status: InProgress pay: free Filter: 428079',
        #      'ts_sla_failed': 'Queue: CLOUDSUPPORT Status: open, InProgress pay: free %s' % SLA_FAILED_QUERY,
        #      'query_type': 'query',
        #      'crew_danger_limit': 1,
        #      'crew_warning_limit': 2,
        #      'ts_danger_limit': 10,
        #      'ts_warning_limit': 7},
        #     {'q_id': uuid4(),
        #      'name': 'Mails',
        #      'ts_open': 'Author: zomb-prj-191@ Queue: CLOUDSUPPORT Status: open, inProgress',
        #      'ts_in_progress': 'Author: zomb-prj-191@ Queue: CLOUDSUPPORT Status: inProgress',
        #      'ts_sla_failed': 'Author: zomb-prj-191@ Queue: CLOUDSUPPORT Status: open,inProgress %s' % SLA_FAILED_QUERY,
        #      'query_type': 'query',
        #      'crew_danger_limit': 1,
        #      'crew_warning_limit': 2,
        #      'ts_danger_limit': 10,
        #      'ts_warning_limit': 7},
        #     {'q_id': uuid4(),
        #      'name': 'Надо разметить',
        #      'ts_open': 'Queue: CLOUDSUPPORT Status: open, InProgress  Components: empty() ',
        #      'ts_in_progress': 'Queue: CLOUDSUPPORT Status: open, InProgress  Components: empty() ',
        #      'ts_sla_failed': 'Queue: CLOUDSUPPORT Status: open, InProgress  Components: empty() %s' % SLA_FAILED_QUERY,
        #      'query_type': 'query',
        #      'crew_danger_limit': 1,
        #      'crew_warning_limit': 2,
        #      'ts_danger_limit': 10,
        #      'ts_warning_limit': 7},
        #     {'q_id': uuid4(),
        #      'name': 'MDB',
        #      'ts_open': 'queue: CLOUDSUPPORT Components: mdb.sqlserver, mdb.greenplum, mdb.clickhouse, '
        #                 'mdb.elasticsearch, mdb.redis, mdb.kafka, mdb.mysql, mdb, mdb.mongodb, mdb.postgresql, '
        #                 'data-transfer  Status: open, inProgress',
        #      'ts_in_progress': 'queue: CLOUDSUPPORT Components: mdb.sqlserver, mdb.greenplum, mdb.clickhouse, '
        #                 'mdb.elasticsearch, mdb.redis, mdb.kafka, mdb.mysql, mdb, mdb.mongodb, mdb.postgresql, '
        #                 'data-transfer  Status: open',
        #      'ts_sla_failed': 'queue: CLOUDSUPPORT Components: mdb.sqlserver, mdb.greenplum, mdb.clickhouse, '
        #                 'mdb.elasticsearch, mdb.redis, mdb.kafka, mdb.mysql, mdb, mdb.mongodb, mdb.postgresql, '
        #                 'data-transfer  Status: open, inProgress %s' % SLA_FAILED_QUERY,
        #      'query_type': 'query',
        #      'crew_danger_limit': 1,
        #      'crew_warning_limit': 2,
        #      'ts_danger_limit': 10,
        #      'ts_warning_limit': 7},
        #     {'q_id': uuid4(),
        #      'name': 'Новые business и standard',
        #      'ts_open': 'Queue: CLOUDSUPPORT Status: open, inProgress  (Modifier: robot-yc-support-api OR Modifier: '
        #                 'spock OR Modifier: robot-cloud-duty ) Tags: ! reopen Tags: !wfa Followers: ! group(value: '
        #                 '\\"Группа поддержки пользователей Яндекс.Облака\\")Followers: ! group(value: \\"Группа ведущих '
        #                 'инженеров поддержки Яндекс.Облака\\")  followers: !preda10r@  followers: !klaisens@ followers: '
        #                 '! anastasiasdk@ followers: !vrmy87@ followers: !sbrusov@ pay: business, standard',
        #      'ts_in_progress': 'Queue: CLOUDSUPPORT Status: inProgress  (Modifier: robot-yc-support-api OR '
        #                        'Modifier: '
        #                 'spock OR Modifier: robot-cloud-duty ) Tags: ! reopen Tags: !wfa Followers: ! group(value: '
        #                 '\"Группа поддержки пользователей Яндекс.Облака\")Followers: ! group(value: \"Группа ведущих '
        #                 'инженеров поддержки Яндекс.Облака\")  followers: !preda10r@  followers: !klaisens@ followers: '
        #                 '! anastasiasdk@ followers: !vrmy87@ followers: !sbrusov@ pay: business, standard',
        #      'ts_sla_failed': 'Queue: CLOUDSUPPORT Status: open, inProgress  (Modifier: robot-yc-support-api OR '
        #                       'Modifier: '
        #                 'spock OR Modifier: robot-cloud-duty ) Tags: ! reopen Tags: !wfa Followers: ! group(value: '
        #                 '\"Группа поддержки пользователей Яндекс.Облака\")Followers: ! group(value: \"Группа ведущих '
        #                 'инженеров поддержки Яндекс.Облака\")  followers: !preda10r@  followers: !klaisens@ followers: '
        #                 '! anastasiasdk@ followers: !vrmy87@ followers: !sbrusov@ '
        #                       'pay: business, standard %s' % SLA_FAILED_QUERY,
        #      'query_type': 'query',
        #      'crew_danger_limit': 1,
        #      'crew_warning_limit': 2,
        #      'ts_danger_limit': 10,
        #      'ts_warning_limit': 3},
        #
        ]

        for filter in filters:
            data = {}
            for k, v in filter.items():
                data[k] = v
            new_filter = Queue_Filter(**data)
            new_filter_srlzr = Queue_FilterSerializer(data=data)
            new_filter_srlzr.is_valid()
            new_filter.save(new_filter_srlzr.validated_data)
