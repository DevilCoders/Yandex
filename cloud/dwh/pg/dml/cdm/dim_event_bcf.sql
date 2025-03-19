set timezone = 'UTC';


do
$$
    begin
        insert into cdm.dim_event
        (name_ru, name_en, date, bcf_event_id, description_ru, description_en, short_description_ru,
         short_description_en,
         image, record, stream, is_canceled, registration_form_id, url, is_published,
         start_registration_time,
         contact_person, contact_person_phone, registration_url, registration_status, is_online, webinar_url,
         need_request_email, place_ru, place_en, city_ru, city_en, address_ru, address_en, _insert_dttm, _update_dttm)
        select e.name_ru,
               e.name_en,
               e.date,
               e.id,
               e.description_ru,
               e.description_en,
               e.short_description_ru,
               e.short_description_en,
               e.image,
               e.record,
               e.stream,
               e.is_canceled,
               e.registration_form_id,
               e.url,
               e.is_published,
               e.start_registration_time,
               e.contact_person,
               e.contact_person_phone,
               e.registration_url,
               e.registration_status,
               e.is_online,
               e.webinar_url,
               e.need_request_email,
               p.name_ru    as place_ru,
               p.name_en    as place_en,
               c.name_ru    as city_ru,
               c.name_en    as city_en,
               p.address_ru as address_ru,
               p.address_en as address_en,
               now(),
               now()
        from ods.bcf_event e
        join ods.bcf_place p
          on e.place_id = p.id
        join ods.bcf_city c
          on c.id = p.city_id
       where not exists(
            select 1 from cdm.dim_event ee where ee.bcf_event_id = e.id
       );

        update cdm.dim_event e1
        set name_ru                 = e.name_ru,
            name_en                 = e.name_en,
            date                    = e.date,
            description_ru          = e.description_ru,
            description_en          = e.description_en,
            short_description_ru    = e.short_description_ru,
            short_description_en    = e.short_description_en,
            image                   = e.image,
            record                  = e.record,
            stream                  = e.stream,
            is_canceled             = e.is_canceled,
            registration_form_id    = e.registration_form_id,
            url                     = e.url,
            is_published            = e.is_published,
            start_registration_time = e.start_registration_time,
            contact_person          = e.contact_person,
            contact_person_phone    = e.contact_person_phone,
            registration_url        = e.registration_url,
            registration_status     = e.registration_status,
            is_online               = e.is_online,
            webinar_url             = e.webinar_url,
            need_request_email      = e.need_request_email,
            place_ru                = p.name_ru,
            place_en                = p.name_en,
            city_ru                 = c.name_ru,
            city_en                 = c.name_en,
            address_ru              = p.address_ru,
            address_en              = p.address_en,
            _update_dttm            = now()
        from ods.bcf_event e
        join ods.bcf_place p
          on e.place_id = p.id
        join ods.bcf_city c
          on c.id = p.city_id
       where e1.bcf_event_id = e.id
         and (
                coalesce(e1.name_ru, '') <> coalesce(e.name_ru, '') or
                coalesce(e1.name_en, '') <> coalesce(e.name_en, '') or
                coalesce(e1.date::varchar, '') <> coalesce(e.date::varchar, '') or
                coalesce(e1.description_ru, '') <> coalesce(e.description_ru, '') or
                coalesce(e1.description_en, '') <> coalesce(e.description_en, '') or
                coalesce(e1.short_description_ru, '') <> coalesce(e.short_description_ru, '') or
                coalesce(e1.short_description_en, '') <> coalesce(e.short_description_en, '') or
                coalesce(e1.image, '') <> coalesce(e.image, '') or
                coalesce(e1.record, '') <> coalesce(e.record, '') or
                coalesce(e1.stream, '') <> coalesce(e.stream, '') or
                coalesce(e1.is_canceled::varchar, '') <> coalesce(e.is_canceled::varchar, '') or
                coalesce(e1.registration_form_id, '') <> coalesce(e.registration_form_id, '') or
                coalesce(e1.url, '') <> coalesce(e.url, '') or
                coalesce(e1.is_published::varchar, '') <> coalesce(e.is_published::varchar, '') or
                coalesce(e1.start_registration_time::varchar, '') <> coalesce(e.start_registration_time::varchar, '') or
                coalesce(e1.contact_person, '') <> coalesce(e.contact_person, '') or
                coalesce(e1.contact_person_phone, '') <> coalesce(e.contact_person_phone, '') or
                coalesce(e1.registration_url, '') <> coalesce(e.registration_url, '') or
                coalesce(e1.registration_status, '') <> coalesce(e.registration_status, '') or
                coalesce(e1.is_online::varchar, '') <> coalesce(e.is_online::varchar, '') or
                coalesce(e1.webinar_url, '') <> coalesce(e.webinar_url, '') or
                coalesce(e1.need_request_email::varchar, '') <> coalesce(e.need_request_email::varchar, '') or
                coalesce(e1.place_ru, '') <> coalesce(p.name_ru, '') or
                coalesce(e1.place_en, '') <> coalesce(p.name_en, '') or
                coalesce(e1.address_en, '') <> coalesce(p.address_en, '') or
                coalesce(e1.address_ru, '') <> coalesce(p.address_ru, '') or
                coalesce(e1.city_en, '') <> coalesce(c.name_en, '') or
                coalesce(e1.city_ru, '') <> coalesce(c.name_ru, '')
         );
    end
$$;
