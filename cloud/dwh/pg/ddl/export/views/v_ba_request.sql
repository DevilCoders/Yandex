create view export.v_ba_request
            ("Timestamp", "CRM_Lead_ID", "Billing_account_id", "Status", "Description", "Assigned_to", "First_name",
             "Last_name", "Phone_1", "Phone_2", "Email", "Lead_Source", "Lead_Source_Description", "Callback_date",
             "Last_communication_date", "Promocode", "Promocode_sum", "Notes", "Dimensions", "Tags")
as
select crm_ba_request.create_dttm as "Timestamp",
       crm_ba_request.crm_lead_id as "CRM_Lead_ID",
       ('["' || crm_ba_request.ba_id || '"]')::varchar(255)   as "Billing_account_id",
       null::text                 as "Status",
       null::text                 as "Description",
       null::text                 as "Assigned_to",
       null::text                 as "First_name",
       null::text                 as "Last_name",
       null::text                 as "Phone_1",
       null::text                 as "Phone_2",
       null::text                 as "Email",
       null::text                 as "Lead_Source",
       null::text                 as "Lead_Source_Description",
       null::text                 as "Callback_date",
       null::text                 as "Last_communication_date",
       null::text                 as "Promocode",
       null::text                 as "Promocode_sum",
       null::text                 as "Notes",
       null::text                 as "Dimensions",
       null::text                 as "Tags"
from export.crm_ba_request;
