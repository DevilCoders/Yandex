create view v_dim_event
            (id, name_ru, name_en, date, description_ru, record, stream, is_canceled, registration_form_id, url,
             start_registration_time, contact_person, registration_url, registration_status, is_online, webinar_url,
             need_request_email, city_ru, place_ru, address_ru)
as
select dim_event.id,
       dim_event.name_ru,
       dim_event.name_en,
       dim_event.date,
       dim_event.description_ru,
       dim_event.record,
       dim_event.stream,
       dim_event.is_canceled,
       dim_event.registration_form_id,
       dim_event.url,
       dim_event.start_registration_time,
       dim_event.contact_person,
       dim_event.registration_url,
       dim_event.registration_status,
       dim_event.is_online,
       dim_event.webinar_url,
       dim_event.need_request_email,
       dim_event.city_ru,
       dim_event.place_ru,
       dim_event.address_ru
from cdm.dim_event;

grant select on export.v_dim_event to "etl-user";

