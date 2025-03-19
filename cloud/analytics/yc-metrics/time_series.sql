DEFINE SUBQUERY  $metric_growth($metric, $params, $color_scheme,$color_type, $target_value) AS
SELECT 
    `date`,
    metric_id,
    metric_name,
    metric_group,
    metric_subgroup,
    metric_unit, 
    is_ytd,
    period,
    CAST(metric_value AS Double) as metric_value,
    LAG(CAST(metric_value AS Double), 1) OVER w as metric_value_prev,
    CAST(metric_value AS Double) - LAG(CAST(metric_value AS Double), 1) OVER w  as metric_growth_abs,
    (CAST(metric_value AS Double) - LAG(CAST(metric_value AS Double), 1) OVER w) / LAG(CAST(metric_value AS Double),1) OVER w as metric_growth_pct,
    CASE 
        WHEN $color_type = 'standart' THEN IF(
            Abs(CAST(metric_value AS Double) - AVG(CAST(metric_value AS Double)) OVER w10) 
            < STDDEV_SAMPLE(CAST(metric_value AS Double)) OVER w10, 'yellow',
                IF(CAST(metric_value AS Double) < AVG(CAST(metric_value AS Double)) OVER w10,
                    IF($color_scheme = 'straight', 'red', 'green'),
                        IF(CAST(metric_value AS Double) < LAG(CAST(metric_value AS Double)) OVER w10,
                            'yellow',
                                IF($color_scheme = 'straight', 'green', 'red')
                        )
                )
            )
        WHEN $color_type = 'goal' THEN IF(
            CAST(metric_value AS Double) > CAST($target_value AS Double)*1.02, 
                IF($color_scheme = 'straight', 'green', 'red'),    
                    IF(
                        CAST(metric_value AS Double) < CAST($target_value AS Double)*0.98,
                            IF($color_scheme = 'straight', 'red', 'green'),
                                'yellow'
                    )
            )
        WHEN $color_type = 'growth' then 'black'
        ELSE 'grey'
    END AS metric_color  
FROM $metric($params)
WINDOW 
    w AS (
        PARTITION BY metric_id, period
        ORDER BY `date`
    ),
    w10 AS (
        PARTITION BY metric_id, period
        ORDER BY `date`
        ROWS BETWEEN 10 PRECEDING AND CURRENT ROW
    )
END DEFINE;

EXPORT $metric_growth;