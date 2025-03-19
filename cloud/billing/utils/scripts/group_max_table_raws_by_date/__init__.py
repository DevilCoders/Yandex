def group_max_table_raws_by_date(yql_client, src_dir_path, dst_dir_path):
    query = """
        PRAGMA yt.InferSchema = '1';
        $dst_dir = "{dst_dir}";

        $max_path = (SELECT MAX(Path) AS Path
            FROM FOLDER("{src_dir}", "row_count")
            WHERE Type = "table");


        $distinct_dates = (SELECT AGGREGATE_LIST(DISTINCT `date`) from CONCAT($max_path));



        DEFINE ACTION $date_action($date) as
            $path = $dst_dir || "/" || $date;
            INSERT INTO $path WITH TRUNCATE SELECT * FROM CONCAT($max_path) WHERE `date` = $date;
        END DEFINE;


        EVALUATE FOR $date in $distinct_dates  DO $date_action($date);
    """
    request = yql_client.query(query.format(src_dir=src_dir_path, dst_dir=dst_dir_path), syntax_version=1)
    request.run()
    result = request.get_results()
    if not result.is_success:
        raise RuntimeError("YQL Query is not succeeded. Operation id: {}".format(str(request.operation_id)))
