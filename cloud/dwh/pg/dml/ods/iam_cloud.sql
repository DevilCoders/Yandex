SET TIMEZONE='UTC';

DO
$$
    BEGIN
        truncate table ods.iam_cloud;
        insert into ods.iam_cloud
        (passport_uid, cloud_id, cloud_name, cloud_status, cloud_created_at, _insert_dttm)
        select passport_uid, cloud_id, cloud_name, cloud_status, cloud_created_at, now() as _insert_dttm
        from stg.iam_cloud_creator;
    END
$$;
