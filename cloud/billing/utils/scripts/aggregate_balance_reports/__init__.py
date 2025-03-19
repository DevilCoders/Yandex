def aggregate_balance_reports(table_path_prefix, date_from, date_to, env, yql_client):
    query = """
        PRAGMA TablePathPrefix = "{table_path_prefix}";
        $date_from = "{date_from}";
        $date_to = "{date_to}";
        $env = "{env}";

        $balance_reports_table = "exported-billing-tables/balance_reports_" || $env;
        $var_adjustments_table = "exported-billing-tables/var_adjustments_" || $env;
        $dst_dir = "balance_cloud_completions_" || $env;

        $format_date = DateTime::Format("%Y-%m-%d");

        $get_date_range_inclusive_str = ($start_date, $end_date) -> {{
          $start_date_dttm = DateTime::MakeDate(DateTime::ParseIso8601($start_date));
          $end_date_dttm = DateTime::MakeDate(DateTime::ParseIso8601($end_date));

          return ListCollect(ListMap(ListFromRange(0, (DateTime::ToDays($end_date_dttm - $start_date_dttm) + 1) ?? 30), ($x) -> {{
              return $format_date($start_date_dttm + DateTime::IntervalFromDays(CAST($x AS Int16)))
          }}));
        }};

        $dates = $get_date_range_inclusive_str($date_from, $date_to);

        $to_decimal = ($amount) -> {{
          RETURN CAST($amount AS Decimal(35,9));
        }};

        $adjustments = (
          SELECT
            `date`,
            billing_account_id,
            sum($to_decimal(amount)) as total
          FROM $var_adjustments_table
          WHERE `date` between $date_from and $date_to
          GROUP BY `date`, billing_account_id
        );

        $reports = (
          SELECT
            `date`,
            billing_account_id,
            balance_product_id,
            sum($to_decimal(total)) as total
          FROM $balance_reports_table
          WHERE `date` between $date_from and $date_to
          GROUP BY `date`, billing_account_id, balance_product_id
        );

        $result = (
          SELECT
            b.`date` as `date`,
            cast(b.total + NVL(v.total, $to_decimal("0")) as string) as amount,
            b.balance_product_id as product_id,
            b.billing_account_id as project_id
          FROM $reports as b
          LEFT
          JOIN $adjustments as v USING (billing_account_id, `date`)
        );

        EVALUATE FOR $date in $dates DO BEGIN
          $dst_table = $dst_dir || "/" || $date;
          INSERT INTO $dst_table WITH TRUNCATE
          SELECT
            $date as `date`,
            amount,
            product_id,
            project_id
          FROM $result
          WHERE `date` = $date
        END DO;
    """

    request = yql_client.query(query.format(
        table_path_prefix=table_path_prefix,
        date_from=date_from,
        date_to=date_to,
        env=env,
    ), syntax_version=1)
    request.run()
    result = request.get_results()
    if not result.is_success:
        raise RuntimeError(
            "YQL Query is not succeeded. Operation id: {}. Errors: \n{}".format(str(request.operation_id), "\n".join(
                issue.format_issue() for issue in result.errors)))
