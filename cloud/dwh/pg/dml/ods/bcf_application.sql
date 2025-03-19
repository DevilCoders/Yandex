DO
$$
    BEGIN
        truncate table ods.bcf_application;
        insert into ods.bcf_application
        (email, id, status, created_at, updated_at, event_id, participant_id, visited, cloud_interest_reason, came_from,
         infra_deployed_in, needs_call)
        select
            email,
            id,
            status,
            created_at,
            updated_at,
            event_id,
            participant_id,
            visited = 1,
            cloud_interest_reason,
            came_from,
            infra_deployed_in,
           case
               when needs_call in ('yes', 'да', 'Yes', 'Да') then true
               when needs_call is null then null
               else false
           end as needs_call
        from (
                 select email,
                        first_value(application_id) over w as id,
                        first_value(status) over w         as status,
                        first_value(created_at) over w     as created_at,
                        first_value(updated_at) over w     as updated_at,
                        first_value(event_id) over w       as event_id,
                        first_value(participant_id) over w as participant_id,
                        first_value(visited) over w        as visited,
                        first_value(needs_call) over w     as needs_call,
                        first_value(cloud_interest_reason) over w as cloud_interest_reason,
                        first_value(came_from) over w             as came_from,
                        first_value(infra_deployed_in) over w     as infra_deployed_in,
                        row_number() over w                as rn
                 from (
                          select participant_id,
                                 application_id,
                                 max(created_at) as created_at,
                                 max(email)      as email,
                                 max(updated_at) as updated_at,
                                 max(event_id)   as event_id,
                                 max(status)     as status,
                                 max(visited::int) as visited,
                                 max(needs_call)   as needs_call,
                                 max(cloud_interest_reason) as cloud_interest_reason,
                                 max(came_from) as came_from,
                                 max(infra_deployed_in) as infra_deployed_in
                          from (
                                   select application_id,
                                          participant_id,
                                          event_id,
                                          created_at,
                                          updated_at,
                                          status,
                                          visited,
                                          case
                                            when field_slug = 'email' then lower(field_value)
                                          end as email,
                                          case
                                            when field_name like '%связался специалист%' then lower(field_value)
                                          end as needs_call,
                                          case
                                              when field_name like '%задачу вы планируете решить с помощью облачных сервисов%' then lower(field_value)
                                          end as cloud_interest_reason,
                                          case
                                              when field_name like '%узнали о мероприятии%' then lower(field_value)
                                          end as came_from,
                                          case
                                              when field_name like '%сейчас развернута ваша%' then lower(field_value)
                                          end as infra_deployed_in
                                   from (
                                            select app.id as application_id,
                                                   app.participant_id,
                                                   app.event_id,
                                                   app.field_name,
                                                   app.field_value,
                                                   app.field_slug,
                                                   app.visited,
                                                   app.created_at,
                                                   app.updated_at,
                                                   app.status
                                            from stg.bcf_application app
                                            where field_slug = 'email'
                                               or field_name like '%связался специалист%'
                                               or field_name like '%задачу вы планируете решить с помощью облачных сервисов%'
                                               or field_name like '%узнали о мероприятии%'
                                               or field_name like '%сейчас развернута ваша%'
                                        ) as t
                               ) as t3
                          group by t3.application_id, t3.participant_id
                      ) as t4
                     window w as (partition by email, event_id, participant_id order by created_at desc)
             ) as t4
        where t4.rn = 1;
  END
$$;

