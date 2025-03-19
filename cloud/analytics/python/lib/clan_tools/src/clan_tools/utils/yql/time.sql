$day_before_current = ($table) -> {
    RETURN DateTime::MakeDate(DateTime::ParseIso8601($table ||"+0300"))
            >= CAST(DateTime::FromSeconds(
                CAST(DateTime::ToSeconds(CurrentUtcDate()) - 86400 AS UInt32))
            AS Date)
};

$date_time = ($str) -> {RETURN DateTime::MakeDatetime(DateTime::Parse('%Y-%m-%d %H:%M:%S')($str))};



$parse_datetime = DateTime::Parse('%Y-%m-%d %H:%M:%S');
$parse_date = DateTime::Parse('%Y-%m-%d');
$format_datetime = DateTime::Format('%Y-%m-%d %H:%M:%S');
$format_date = DateTime::Format('%Y-%m-%d');
$format_from_second = ($event_time) -> {
    RETURN DateTime::FromSeconds(CAST ($event_time AS Uint32))
}; 
$format_from_microsecond = ($event_time) -> {
    RETURN DateTime::FromMicroseconds(CAST ($event_time AS Uint64))
}; 

$round_period_datetime = ($period,$event_time) -> { 
    RETURN CASE 
        WHEN $period = 'day' then $format_date(DateTime::StartOfDay($parse_datetime($event_time)))
        WHEN $period = 'week' then $format_date(DateTime::StartOfWeek($parse_datetime($event_time)))
        WHEN $period = 'month' then $format_date(DateTime::StartOfMonth($parse_datetime($event_time)))
        WHEN $period = 'quarter' then $format_date(DateTime::StartOfQuarter($parse_datetime($event_time)))
        WHEN $period = 'year' then $format_date(DateTime::StartOfYear($parse_datetime($event_time)))
        ELSE NULL 
    END ;
};

$round_period_date = ($period,$event_time) -> { 
    RETURN CASE 
        WHEN $period = 'day' then $format_date(DateTime::StartOfDay($parse_date($event_time)))
        WHEN $period = 'week' then $format_date(DateTime::StartOfWeek($parse_date($event_time)))
        WHEN $period = 'month' then $format_date(DateTime::StartOfMonth($parse_date($event_time)))
        WHEN $period = 'quarter' then $format_date(DateTime::StartOfQuarter($parse_date($event_time)))
        WHEN $period = 'year' then $format_date(DateTime::StartOfYear($parse_date($event_time)))
        ELSE NULL 
    END ;
};

$round_period_format = ($period,$format) -> { 
    RETURN CASE 
        WHEN $period = 'day' then $format_date(DateTime::StartOfDay($format))
        WHEN $period = 'week' then $format_date(DateTime::StartOfWeek($format))
        WHEN $period = 'month' then $format_date(DateTime::StartOfMonth($format))
        WHEN $period = 'quarter' then $format_date(DateTime::StartOfQuarter($format))
        WHEN $period = 'year' then $format_date(DateTime::StartOfYear($format))
        ELSE NULL 
    END ;
};

$round_current_time = ($period) -> {
    RETURN CASE 
        WHEN $period = 'day' then $format_date(DateTime::StartOfDay(CurrentUtcDatetime()))
        WHEN $period = 'week' then $format_date(DateTime::StartOfWeek(CurrentUtcDatetime()))
        WHEN $period = 'month' then $format_date(DateTime::StartOfMonth(CurrentUtcDatetime()))
        WHEN $period = 'quarter' then $format_date(DateTime::StartOfQuarter(CurrentUtcDatetime()))
        WHEN $period = 'year' then $format_date(DateTime::StartOfYear(CurrentUtcDatetime()))
        ELSE NULL 
    END ;
};


$script = @@
from datetime import datetime, timedelta
def dates_range(d0_ = '2018-01-01', d1_ = datetime.now().strftime('%Y-%m-%d')):
    d0 = d0_.decode('UTF-8')
    d1 = d1_.decode('UTF-8') 
    return [(datetime.strptime(d0, '%Y-%m-%d') + timedelta(days=i)).strftime('%Y-%m-%d') for i in range((datetime.strptime(d1, '%Y-%m-%d') - datetime.strptime(d0, '%Y-%m-%d') ).days+1)]
@@;

$dates_range = Python3::dates_range(
    Callable<(String, String)->List<String>>,

    $script
);

EXPORT 
    $day_before_current, 
    $date_time, 
    $parse_datetime, 
    $parse_date, 
    $format_datetime, 
    $format_date, 
    $dates_range,
    $round_period_datetime,
    $round_period_date,
    $round_current_time,
    $round_period_format,
    $format_from_second,
    $format_from_microsecond
    ;
