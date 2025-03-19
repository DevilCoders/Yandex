INSERT INTO "<append=%false>{table}"

SELECT 
  date,
  costs_type,
  costs*c as costs,
  billing_account_id
FROM (
SELECT
  
  toString(date) as date,
  
  'Support' as costs_type,
  w_ * {r1} * 8 * 247/365  as costs,
  billing_account_id
FROM
  (
    SELECT
      date,
      groupArray(billing_account_id) as billing_account_id,
      groupArray(first_first_paid_consumption_datetime) as first_first_paid_consumption_datetime,
      sum(w) as sum_w_,
      arrayMap(x -> x / sum_w_, groupArray(w)) as w_
    FROM(
        SELECT date, billing_account_id, first_first_paid_consumption_datetime, sum(w) as w 
FROM (
SELECT
          created as date,
          if(
            assumeNotNull(billing_account_id) = '',
            'undefined',
            billing_account_id
          ) as billing_account_id,
          coalesce(first_first_paid_consumption_datetime, '2099-12-31 00:00:00') as first_first_paid_consumption_datetime,
          w
        FROM
          (
            SELECT
              id,
              key,
              toDate(created / 1000) as created,
              comp,
              emailFrom,
              emailFromUser,
              billing_account_id,
              cloud_id,
              name,
              rank_ as rank,
              time,
              if(c > 0, c, 1) as c,
              c * time as w
            FROM
              (
                SELECT
                  id,
                  key,
                  argMax(created, rank) as created,
                  argMax(comp, rank) as comp,
                  argMax(emailFrom, rank) as emailFrom,
                  argMax(emailFromUser, rank) as emailFromUser,
                  argMax(billing_account_id, rank) as billing_account_id,
                  argMax(cloud_id, rank) as cloud_id,
                  argMax(name, rank) as name,
                  max(rank) as rank_,
                  argMax(time, rank) as time
                FROM
                  (
                    SELECT
                      id,
                      key,
                      created,
                      comp,
                      emailFrom,
                      emailFromUser,
                      billing_account_id,
                      cloud_id,
                      name,
                      toUInt32(coalesce(rank, '5')) as rank,
                      toUInt32(coalesce(time, '5')) as time
                    FROM
                      (
                        SELECT
                          id,
                          key,
                          created,
                          comp,
                          emailFrom,
                          emailFromUser,
                          billing_account_id,
                          cloud_id,
                          name
                        FROM
                          (
                            SELECT
                              id,
                              key,
                              created,
                              comp,
                              emailFrom,
                              emailFromUser,
                              if(
                                assumeNotNull(billing_account_id) = '',
                                assumeNotNull(b.billing_account_id),
                                billing_account_id
                              ) as billing_account_id,
                              cloud_id
                            FROM
                              (
                                SELECT
                                  id,
                                  key,
                                  created,
                                  arrayMap(
                                    x -> substring(x, 3),
                                    splitByChar(
                                      ';',
                                      substring(
                                        assumeNotNull(components),
                                        2,
                                        length(assumeNotNull(components)) -2
                                      )
                                    )
                                  ) as comp,
                                  --customFields,
                                  emailFrom,
                                  replaceAll(
                                    substring(emailFrom, 1, position(emailFrom, '@') - 1),
                                    '.',
                                    '-'
                                  ) as emailFromUser,
                                  YPathString(customFields, '/billingId') as billing_account_id,
                                  YPathString(customFields, '/cloudId') as cloud_id
                                FROM
                                  "//home/cloud_analytics/import/startrek/support/issues"
                                WHERE
                                  1 = 1
                              ) as a
                              LEFT JOIN (
                                SELECT
                                  user_settings_email as email,
                                  billing_account_id
                                FROM
                                  "//home/cloud_analytics/cubes/acquisition_cube/cube"
                                GROUP BY 
                                    user_settings_email, billing_account_id
                              ) as b ON a.emailFrom = b.email ARRAY
                              JOIN comp
                          ) as x
                          LEFT JOIN "//home/cloud_analytics/import/startrek/support/components" as y ON x.comp = y.id
                      ) as xy
                      LEFT JOIN "//home/cloud_analytics/tmp/artkaz/unit_economy/support_weights" as z ON xy.name = z.component
                  )
                GROUP BY
                  id,
                  key
              ) as p
              LEFT JOIN (
                SELECT
                  issue,
                  count(id) as c
                FROM
                  "//home/cloud_analytics/import/startrek/support/comments"
                GROUP BY
                  issue
              ) as q ON p.id = q.issue
          ) as a LEFT JOIN (
            SELECT 
                billing_account_id,
                max(if(toString(first_first_paid_consumption_datetime)='', '2099-12-31 00:00:00', first_first_paid_consumption_datetime)) as first_first_paid_consumption_datetime
            FROM 
                 "//home/cloud_analytics/cubes/acquisition_cube/cube"
            GROUP BY billing_account_id
                ) as b 
            USING billing_account_id
        WHERE
          --date >= '2019-01-01'
          assumeNotNull(billing_account_id) != 'undefined'
)
GROUP BY date, billing_account_id, first_first_paid_consumption_datetime
      )
    GROUP BY
      date
      
  ) ARRAY
  JOIN billing_account_id,
  first_first_paid_consumption_datetime,
  w_
ORDER BY
  date DESC
) as a LEFT JOIN 
"//home/cloud_analytics/unit_economy/support_costs/support_count_cum" as b 
ON a.date = b.d