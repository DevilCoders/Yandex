-- constants
$SECOND = 1u;
$MINUTE = 60u * $SECOND;
$HOUR = 60u * $MINUTE;
$DAY = 24u * $HOUR;

$DATETIME_INF = CAST("2105-12-31T23:59:59Z" as DateTime);
$DATETIME_NEG_INF = CAST("1970-01-01T00:00:00Z" as DateTime);

$UTC_TIMEZONE = "UTC";
$MSK_TIMEZONE = "Europe/Moscow";

-- format datetime to date string
$format_date = DateTime::Format("%Y-%m-%d");
$format_datetime = DateTime::Format("%Y-%m-%d %H:%M:%S");
$format_time = DateTime::Format("%H:%M:%S");
$format_iso8601 = DateTime::Format("%Y-%m-%dT%H:%M:%S%z");
$format_hour = DateTime::Format("%Y-%m-%dT%H:00:00");
$format_month = DateTime::Format("%Y-%m-01");
$format_month_cohort = DateTime::Format("%Y-%m");
$format_month_name = ($dttm) -> (DateTime::GetMonthName($dttm));
$format_year = DateTime::Format("%Y");
$format_quarter = ($dttm) -> ($format_year($dttm) || '-Q' || Unwrap(CAST(((DateTime::GetMonth($dttm) - 1) / 3 + 1) as String)));
$format_half_year = ($dttm) -> ($format_year($dttm) || '-H' || Unwrap(CAST(((DateTime::GetMonth($dttm) - 1) / 6 + 1) as String)));

-- convert days to timestamp
$get_timestamp_from_days = ($days) -> ($days * 24 * 60 * 60);

-- convert timestamp to DateTime without timezone
$get_date = ($ts) -> (CAST($ts AS Date));
$get_datetime = ($ts) -> (CAST($ts AS Datetime));
$get_datetime_ms = ($ts) -> (DateTime::FromMilliseconds($ts));
$get_datetime_secs = ($ts) -> (DateTime::FromSeconds($ts));

-- convert timestamp to DateTime with timezone
$get_tz_datetime = ($ts, $tz) -> (AddTimezone($get_datetime($ts), $tz));
$get_utc_datetime = ($ts) -> ($get_tz_datetime($ts, $UTC_TIMEZONE));
$get_msk_datetime = ($ts) -> ($get_tz_datetime($ts, $MSK_TIMEZONE));

-- convert timestamp to special format with timezone
$format_by_timestamp = ($ts, $tz, $fmt) -> ($fmt($get_tz_datetime($ts, $tz)));

-- convert timestamp to date string (e.g. 2020-12-31) with timezone
$format_date_by_timestamp = ($ts, $tz) -> ($format_by_timestamp($ts, $tz, $format_date));
$format_utc_date_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $UTC_TIMEZONE, $format_date));
$format_msk_date_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $MSK_TIMEZONE, $format_date));

-- convert timestamp to datetime string (e.g. 2020-12-31 03:00:00) with timezone
$format_datetime_by_timestamp = ($ts, $tz) -> ($format_by_timestamp($ts, $tz, $format_datetime));
$format_utc_datetime_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $UTC_TIMEZONE, $format_datetime));
$format_msk_datetime_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $MSK_TIMEZONE, $format_datetime));

-- convert timestamp to time string (e.g. 03:00:00) with timezone
$format_time_by_timestamp = ($ts, $tz) -> ($format_by_timestamp($ts, $tz, $format_time));
$format_utc_time_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $UTC_TIMEZONE, $format_time));
$format_msk_time_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $MSK_TIMEZONE, $format_time));

-- convert timestamp to Iso8601 string (e.g. 2020-12-31T03:11:59+0300) with timezone
$format_iso8601_by_timestamp = ($ts, $tz) -> ($format_by_timestamp($ts, $tz, $format_iso8601));
$format_utc_iso8601_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $UTC_TIMEZONE, $format_iso8601));
$format_msk_iso8601_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $MSK_TIMEZONE, $format_iso8601));

-- convert timestamp to hour string (e.g. 2020-12-31T03:00:00) with timezone
$format_hour_by_timestamp = ($ts, $tz) -> ($format_by_timestamp($ts, $tz, $format_hour));
$format_utc_hour_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $UTC_TIMEZONE, $format_hour));
$format_msk_hour_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $MSK_TIMEZONE, $format_hour));

-- convert timestamp to month string (e.g. 2020-12-01) with timezone
$format_month_by_timestamp = ($ts, $tz) -> ($format_by_timestamp($ts, $tz, $format_month));
$format_utc_month_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $UTC_TIMEZONE, $format_month));
$format_msk_month_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $MSK_TIMEZONE, $format_month));

-- convert timestamp to month string (e.g. 2020-12) with timezone
$format_month_cohort_by_timestamp = ($ts, $tz) -> ($format_by_timestamp($ts, $tz, $format_month_cohort));
$format_utc_month_cohort_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $UTC_TIMEZONE, $format_month_cohort));
$format_msk_month_cohort_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $MSK_TIMEZONE, $format_month_cohort));

-- convert timestamp to month string (e.g. January) with timezone
$format_month_name_by_timestamp = ($ts, $tz) -> ($format_by_timestamp($ts, $tz, $format_month_name));
$format_utc_month_name_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $UTC_TIMEZONE, $format_month_name));
$format_msk_month_name_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $MSK_TIMEZONE, $format_month_name));

-- convert timestamp to quarter string (e.g. 2020-Q3) with timezone
$format_quarter_by_timestamp = ($ts, $tz) -> ($format_by_timestamp($ts, $tz, $format_quarter));
$format_utc_quarter_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $UTC_TIMEZONE, $format_quarter));
$format_msk_quarter_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $MSK_TIMEZONE, $format_quarter));

-- convert timestamp to half-year string (e.g. 2020-H1) with timezone
$format_half_year_by_timestamp = ($ts, $tz) -> ($format_by_timestamp($ts, $tz, $format_half_year));
$format_utc_half_year_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $UTC_TIMEZONE, $format_half_year));
$format_msk_half_year_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $MSK_TIMEZONE, $format_half_year));

-- convert timestamp to year string (e.g. 2020) with timezone
$format_year_by_timestamp = ($ts, $tz) -> ($format_by_timestamp($ts, $tz, $format_year));
$format_utc_year_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $UTC_TIMEZONE, $format_year));
$format_msk_year_by_timestamp = ($ts) -> ($format_by_timestamp($ts, $MSK_TIMEZONE, $format_year));

-- return next string date (e.g 2020-12-31 -> 2021-01-01)
$get_next_date = ($dt) -> (CAST((CAST($dt as Date) + Interval('P1D')) as String));

-- return inclusive date range with String inputs
$get_date_range_inclusive = ($start_date, $end_date) -> {
  $start_date_dt = Unwrap(NVL(Cast($start_date as TzDate), Cast($start_date as Date)));
  $end_date_dt = Unwrap(NVL(Cast($end_date as TzDate), Cast($end_date as Date)));

  return ListCollect(ListMap(ListFromRange(0, (DateTime::ToDays($end_date_dt - $start_date_dt) + 1) ?? 30), ($x) -> {
      return $format_date($start_date_dt + DateTime::IntervalFromDays(CAST($x AS Int16)))
  }));
};

-- return inclusive month range with String inputs
$get_month_range_inclusive = ($start_date, $end_date) -> {
  $start_date_dt = Unwrap(NVL(Cast($start_date as TzDate), Cast($start_date as Date)));
  $end_date_dt = Unwrap(NVL(Cast($end_date as TzDate), Cast($end_date as Date)));

  return ListCollect(ListSort(ListUniq(ListMap(ListFromRange(0, (DateTime::ToDays($end_date_dt - $start_date_dt) + 1) ?? 30), ($x) -> {
      return $format_month($start_date_dt + DateTime::IntervalFromDays(CAST($x AS Int16)))
  }))));
};


$parse_iso8601_to_datetime_msk = ($datetime_str) -> (
    AddTimeZone(DateTime::MakeDatetime(DateTime::ParseIso8601($datetime_str)), $MSK_TIMEZONE)
);

$parse_hour_date_format_to_ts_msk = ($date_time_str) -> (
    DateTime::MakeTzTimestamp(
        DateTime::Update(
            DateTime::Parse("%Y-%m-%dT%H:00:00")($date_time_str),
            $MSK_TIMEZONE as Timezone
        )
    )
);

$parse_iso8601_string = ($ts) -> (DateTime::MakeTimestamp(DateTime::ParseIso8601($ts)));

$from_utc_ts_to_msk_dt = ($ts) -> (DateTime::MakeDatetime(DateTime::Update(AddTimeZone($ts, "Europe/Moscow"), "GMT"  as Timezone)));
$from_int64_to_ts = ($int) -> (DateTime::FromMilliseconds(cast($int as Uint64)));


EXPORT $SECOND, $MINUTE, $HOUR, $DAY, $UTC_TIMEZONE, $MSK_TIMEZONE, $DATETIME_INF, $DATETIME_NEG_INF,
  $format_date, $format_datetime, $format_time, $format_iso8601, $format_hour, $format_year,
  $format_half_year, $format_quarter, $format_month, $format_month_cohort, $format_month_name,
  $get_tz_datetime, $get_utc_datetime, $get_msk_datetime, $format_by_timestamp, $format_date_by_timestamp, $format_msk_date_by_timestamp,
  $format_datetime_by_timestamp, $format_utc_datetime_by_timestamp, $format_msk_datetime_by_timestamp,
  $format_time_by_timestamp, $format_msk_time_by_timestamp, $format_iso8601_by_timestamp, $format_msk_iso8601_by_timestamp,
  $format_hour_by_timestamp, $format_msk_hour_by_timestamp, $format_quarter_by_timestamp, $format_msk_quarter_by_timestamp,
  $format_month_by_timestamp, $format_msk_month_by_timestamp, $format_utc_month_by_timestamp,
  $format_month_cohort_by_timestamp, $format_msk_month_cohort_by_timestamp, $format_utc_month_cohort_by_timestamp,
  $format_month_name_by_timestamp, $format_msk_month_name_by_timestamp,
  $format_half_year_by_timestamp, $format_msk_half_year_by_timestamp, $format_year_by_timestamp, $format_msk_year_by_timestamp,
  $get_next_date, $get_date_range_inclusive, $get_month_range_inclusive,
  $format_utc_date_by_timestamp, $format_utc_time_by_timestamp, $format_utc_iso8601_by_timestamp,
  $format_utc_hour_by_timestamp, $format_utc_month_name_by_timestamp, $format_utc_quarter_by_timestamp,
  $format_utc_half_year_by_timestamp, $format_utc_year_by_timestamp, $get_date, $get_datetime, $get_datetime_ms, $get_datetime_secs,
  $get_timestamp_from_days, $parse_hour_date_format_to_ts_msk, $parse_iso8601_to_datetime_msk, $parse_iso8601_string,
  $from_utc_ts_to_msk_dt, $from_int64_to_ts;
