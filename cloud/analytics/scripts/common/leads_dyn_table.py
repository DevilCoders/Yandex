import uuid
import time

import yt.wrapper as yt
from crypta.lib.python.identifiers import identifiers as id_lib

iam_to_dyn_mapping = {
    'ba_created_at': 'mkto_ba_created_at',
    'ba_paid_at': 'mkto_ba_paid_at',
    'ba_paid_real_consumption_prob': 'mkto_ba_paid_real_consumption_prob',
    'ba_status': 'mkto_ba_status',
    'billing_account_id': 'ba_id',
    'city': 'mkto_city',
    'client_name': 'mkto_client_name',
    'company': 'mkto_company',
    'console_registration_date': 'mkto_console_registration_date',
    'user_settings_email': 'user_settings_email',
    'first_consumption_date': 'mkto_first_consumption_date',
    'first_name': 'first_name',
    'first_paid_consumption_date': 'mkto_first_paid_consumption_date',
    'grant1_amount': 'mkto_grant1_amount',
    'grant1_end_date': 'mkto_grant1_end_date',
    'grant1_id': 'mkto_grant1_id',
    'grant1_remaining': 'mkto_grant1_remaining',
    'grant1_start_date': 'mkto_grant1_start_date',
    'industry': 'mkto_industry',
    'is_individual': 'mkto_is_individual',
    'last_name': 'last_name',
    'lead_source': 'mkto_lead_source',
    'mail_billing': 'mkto_mail_billing',
    'mail_event': 'mkto_mail_event',
    'mail_feature': 'mkto_mail_feature',
    'mail_info': 'mkto_mail_info',
    'mail_marketing': 'mkto_mail_marketing',
    'mail_promo': 'mkto_mail_promo',
    'mail_support': 'mkto_mail_support',
    'mail_tech': 'mkto_mail_tech',
    'mail_technical': 'mkto_mail_technical',
    'mail_testing': 'mkto_mail_testing',
    'phone': 'mkto_phone',
    'timezone': 'mkto_timezone',
    'person_name': 'mkto_person_name',
    'person_type': 'mkto_person_type',
    'passport_uid': 'passport_uid',
    'sales_person': 'mkto_sales_person',
    'segment': 'mkto_segment',
    'test_group_index': 'mkto_test_group_index',
    'user_settings_language': 'preferred_language',
    'website': 'website',
}

crm_to_dyn_mapping = {
    'account_description': 'crm_account_description',
    'account_id': 'account_id',
    'account_name': 'crm_account_name',
    'alt_address_city': 'crm_alt_address_city',
    'alt_address_country': 'crm_alt_address_country',
    'alt_address_postalcode': 'crm_alt_address_postalcode',
    'alt_address_state': 'crm_alt_address_state',
    'alt_address_street': 'crm_alt_address_street',
    'assigned_user_login': 'crm_assigned_user_login',
    'ba_id': 'ba_id',
    'birthdate': 'crm_birthdate',
    'contact_id': 'crm_contact_id',
    'converted': 'crm_converted',
    'deleted': 'crm_deleted',
    'department': 'crm_department',
    'description': 'crm_description',
    'dimensions': 'crm_dimensions',
    'do_not_call': 'crm_do_not_call',
    'do_not_mail': 'crm_do_not_mail',
    'first_name': 'first_name',
    'id': 'crm_id',
    'last_communication_date': 'crm_last_communication_date',
    'last_name': 'last_name',
    'lead_priority': 'crm_lead_priority',
    'lead_source': 'crm_lead_source',
    'lead_source_description': 'crm_lead_source_description',
    'lead_weight': 'crm_lead_weight',
    'mkto_id': 'mkto_id',
    'opportunity_amount': 'crm_opportunity_amount',
    'opportunity_id': 'crm_opportunity_id',
    'opportunity_name': 'crm_opportunity_name',
    'passport_uid': 'passport_uid',
    'phone_home': 'crm_phone_home',
    'phone_mobile': 'crm_phone_mobile',
    'phone_other': 'crm_phone_other',
    'phone_work': 'crm_phone_work',
    'preferred_language': 'preferred_language',
    'primary_address_city': 'crm_primary_address_city',
    'primary_address_country': 'crm_primary_address_country',
    'primary_address_postalcode': 'crm_primary_address_postalcode',
    'primary_address_state': 'crm_primary_address_state',
    'primary_address_street': 'crm_primary_address_street',
    'status': 'crm_status',
    'status_description': 'crm_status_description',
    'title': 'crm_title',
    'website': 'website',
    'primary_email': 'crm_email',
    'date_modified': 'crm_date_modified'
}

forms_to_dyn_mapping = {
    'created_time': 'forms_created_time',
    'yandex_puid': 'passport_uid',
    'company': 'forms_company',
    'ba_id': 'ba_id',
    'phone': 'forms_phone',
    'email': 'forms_email',
    'position': 'forms_position',
    'lastName': 'last_name',
    'firstName': 'first_name',
    'timezone': 'forms_timezone',
    'mail_marketing': 'forms_mail_marketing',
    'agreementNewsletters': 'forms_mail_marketing',
    'unsubscribe': 'forms_unsubscribe',
    'mail_news': 'mail_news'
}


def index_table_name(table_name, field, pk):
    return f'{table_name}_{field}_{pk}'


class Lead:
    # TODO(syndicut): This can be deduced from schema, maybe by refactoring into full-fledged ORM
    # it's also messy now, mixing validation and data access
    PK = 'dwh_id'
    INT_FIELDS = ['passport_uid', 'mkto_lead_id']
    INDEXED_FIELDS = ['passport_uid', 'email', 'crm_id', 'mkto_lead_id']
    REQUIRED_FIELDS = ['email', 'dwh_updated_at']
    UPDATED_AT_FIELD = 'dwh_updated_at'
    SELECT_FIELDS = [PK] + REQUIRED_FIELDS

    def __init__(self, table_name, yt_client, **kwargs):
        self._table_name = table_name
        self._yt_client = yt_client
        self._data = kwargs
        self._updated = False
        self._reindex_fields = []

    def __getattr__(self, item):
        return self._data.get(item, None)

    def __setattr__(self, key, value):
        if key.startswith('_'):
            return super(Lead, self).__setattr__(key, value)

        value = self.clean(key, value)
        if key not in self._data or value != self._data[key]:
            self._data[key] = value
            self._updated = True
            if key in Lead.INDEXED_FIELDS and value is not None:
                self._reindex_fields.append(key)

    def _index_table(self, index_field):
        return index_table_name(self._table_name, index_field, self.PK)

    def _insert_rows(self):
        yt.insert_rows(self._table_name, (self._data, ), client=self._yt_client, update=True)
        for field in self._reindex_fields:
            index_row = {field: self._data[field], self.PK: self._data[self.PK]}
            yt.insert_rows(self._index_table(field), (index_row, ), client=self._yt_client)

    def _save(self, with_transaction, with_update):
        if with_update:
            self._data[self.UPDATED_AT_FIELD] = int(time.time())
        if self.PK not in self._data:
            self._data[self.PK] = str(uuid.uuid4())
        if with_transaction:
            with self._yt_client.Transaction(type='tablet'):
                self._insert_rows()
        else:
            self._insert_rows()
        return True

    def save(self, with_transaction=True, with_update=True):
        if self._updated:
            return self._save(with_transaction, with_update)

    def from_dict(self, row, mapping=None, non_rewritable_fields=[]):
        for field, value in row.items():
            if mapping:
                if field in mapping:
                    field = mapping[field]
                else:
                    continue

            if field in self.INT_FIELDS:
                if not value:
                    value = None
                else:
                    value = int(value)

            if field not in non_rewritable_fields or getattr(self, field) is None:
                setattr(self, field, value)

    @classmethod
    def clean(cls, key, value):
        return getattr(cls, f'clean_{key}', lambda v: v)(value)

    @classmethod
    def clean_email(cls, email):
        identifier = id_lib.Email(email)
        # print(f"before clean: {email}")
        # print(f"after clean: {identifier.normalize}")
        return identifier.normalize
