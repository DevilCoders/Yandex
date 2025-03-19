create or replace view export.v_crm_lead_event_attendance
(id, first_name, last_name, email, phone, company, position, ba_id, mail_marketing, needs_call,
 registration_status, visited, event_date, event_name, event_link, event_form_id, event_city, cloud_interest_reason,
 event_is_online, lead_source, created_at, lead_source_description)
as
select id,
       first_name,
       last_name,
       email,
       phone,
       company,
       position,
       ba_id,
       mail_marketing,
       needs_call,
       registration_status,
       visited,
       event_date,
       event_name,
       event_link,
       event_form_id,
       event_city,
       cloud_interest_reason,
       event_is_online,
       lead_source,
       created_at,
       lead_source_description
from export.crm_lead_event_attendance
