SET TIMEZONE='UTC';

DO
$$
BEGIN
    -- insert new
    insert into cdm.fct_lead_event_attendance
    (lead_id, event_id, registration_status, visited, needs_call, cloud_interest_reason, came_from, infra_deployed_in,
     created_at, _insert_dttm, _update_dttm)
    select l.id       as lead_id,
           e.id       as event_id,
           apl.status as registration_status,
           apl.visited,
           apl.needs_call,
           apl.cloud_interest_reason,
           apl.came_from,
           apl.infra_deployed_in,
           apl.created_at,
           now()      as _insert_dttm,
           now()      as _update_dttm
    from (
           select a.*, row_number() over (partition by email, event_id order by created_at desc)  as rn
           from ods.bcf_application a
       ) apl
        join (
            select a.*, row_number() over (partition by email order by created_at desc)  as rn
              from ods.bcf_applicant a
        ) a
          on apl.email = a.email
         and apl.rn = 1
         and a.rn = 1
        join cdm.dim_event e
          on e.bcf_event_id = apl.event_id
        join cdm.dim_lead l
          on l.email_bcf = a.email
    where not exists (
        select 1 from cdm.fct_lead_event_attendance le where le.event_id = e.id and le.lead_id = l.id
    ) ;

    -- update old
    update cdm.fct_lead_event_attendance le
       set registration_status = apl.status,
           visited = apl.visited,
           needs_call = apl.needs_call,
           cloud_interest_reason = apl.cloud_interest_reason,
           came_from = apl.came_from,
           infra_deployed_in = apl.infra_deployed_in,
           created_at = apl.created_at,
           _update_dttm = now()
    from (
           select a.*, row_number() over (partition by email, event_id order by created_at desc)  as rn
           from ods.bcf_application a
       ) apl
        join (
            select a.*, row_number() over (partition by email order by created_at desc)  as rn
              from ods.bcf_applicant a
        ) a
          on apl.email = a.email
         and apl.rn = 1
         and a.rn = 1
        join cdm.dim_event e
          on e.bcf_event_id = apl.event_id
        join cdm.dim_lead l
          on l.email_bcf = a.email
    where le.event_id = e.id and le.lead_id = l.id
      and (
           coalesce(le.registration_status, '') <> coalesce(apl.status, '') or
           coalesce(le.visited::varchar, '') <> coalesce(apl.visited::varchar, '') or
           coalesce(le.needs_call::varchar, '') <> coalesce(apl.needs_call::varchar, '') or
           coalesce(le.created_at::varchar, '') <> coalesce(apl.created_at::varchar, '') or
           coalesce(le.cloud_interest_reason::varchar, '') <> coalesce(apl.cloud_interest_reason::varchar, '') or
           coalesce(le.came_from::varchar, '') <> coalesce(apl.came_from::varchar, '') or
           coalesce(le.infra_deployed_in::varchar, '') <> coalesce(apl.infra_deployed_in::varchar, '')
      )
    ;
    END
$$;
