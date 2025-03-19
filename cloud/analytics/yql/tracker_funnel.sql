use chyt.hahn/cloud_analytics_datalens;

DROP TABLE IF EXISTS "//home/cloud_analytics/data_swamp/projects/tracker/funnel_admin";
CREATE TABLE "//home/cloud_analytics/data_swamp/projects/tracker/funnel_admin" ENGINE=YtTable() AS


with admins as (
select  UserID, 
min(toDate(EventDate)) as cohort

from "//home/cloud_analytics/data_swamp/projects/tracker/hit-log_010322_180522" 

where 1=1

and trim(both '/0123456789-/' from path(URL)) = 'hi-there/create' 

group by UserID

),

admins1 as (
select distinct UserID 
from "//home/cloud_analytics/data_swamp/projects/tracker/hit-log_010322_180522"  
where 1=1

and like(Params, '%%{\"hi-there\":\"Успешное создание организации\"}%%')

),

admins2 as (
select distinct  UserID
from "//home/cloud_analytics/data_swamp/projects/tracker/hit-log_010322_180522"  
where 1=1

and like(Params, '%%{\"hi-there\":\"Экран ожидания\"}%%')  
),

admins3 as (
select distinct  UserID 
from "//home/cloud_analytics/data_swamp/projects/tracker/hit-log_010322_180522"  
where 1=1

and like(Params, '%%Переход на главную после подключения%%')  

), 

onboard as (
select distinct  UserID ,
    multiIf (
            like(path(assumeNotNull(URL)), '%%admin/queue/%%'), 
                        'admin/queue', 

            path(assumeNotNull(URL)) = upper(path(assumeNotNull(URL))) 
                and like(path(assumeNotNull(URL)),'%%-%%')=1,
                        'tickets',

            URLPathHierarchy(assumeNotNull(URL))[1] = upper(URLPathHierarchy(assumeNotNull(URL))[1]) 
                and URLPathHierarchy(assumeNotNull(URL))[1] != ''
                and like(URLPathHierarchy(assumeNotNull(URL))[1],'%%-%%')=0,
                    'ORGs',

            trim(both '/0123456789/-' FROM path(assumeNotNull(URL))) 
        ) from_

from "//home/cloud_analytics/data_swamp/projects/tracker/hit-log_010322_180522"  
where 1=1

and 

(
like(Params, '%%Открытие формы добавления пользователя%%')   

or from_ in ('createTicket', 'queues/new', 'queues', 'issues', 'agile/board', 
            'agile/boards/new', 'agile/boards', 'admin/licenses/users', 'admin/statuses', 
            'settings', 'tickets', 'ORGs', 'admin/queue', 'filters/gantt', 'subscriptions', 
            'admin/resolutions', 
            'admin/types', 'admin/fields', 'admin/repositories', 'queues/new/presets/', 'pages/projects/')
                
                )


),

orgs as (

select distinct UserID,  
toInt32(splitByChar(',',splitByChar(':',splitByString('org-', cast(Params,'String'))[2])[1])[1]) org_id
from "//home/cloud_analytics/data_swamp/projects/tracker/hit-log_010322_180522"
where like(Params, '%%"organization":"org-%%')

),

add_user1 as (
select distinct UserID
from "//home/cloud_analytics/data_swamp/projects/tracker/hit-log_010322_180522" 

where 1=1

and (
URL like '%%AddUserTracker%%'
or like(Params, '%%Сохранение приглашенных пользователей%%')      
            )  

),

add_user as (

    select distinct UserID, org_id
    from add_user1 puk join orgs on orgs.UserID=puk.UserID 
)

select 
admins.cohort cohort,
toFloat64(count(distinct admins.UserID) ) as n_peoples,
toFloat64(count(distinct admins1.UserID))  as adminss,
toFloat64(count(distinct admins2.UserID))  as admins_pendings,
toFloat64(count(distinct admins3.UserID))  as admins_dashs,

toFloat64(count(distinct onboard.UserID))  as played_with_trackers,
toFloat64(count(distinct add_user.org_id))  as add_users

from  admins 
left join admins1 ON admins1.UserID = admins.UserID 
left join admins2 ON admins1.UserID = admins2.UserID 
left join admins3 ON admins2.UserID = admins3.UserID 
left join onboard ON admins3.UserID = onboard.UserID 
left join add_user ON onboard.UserID = add_user.UserID 

group by cohort
order by cohort asc
