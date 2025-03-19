CREATE OR REPLACE FUNCTION code.salt2json_path(
    i_path  text
) RETURNS text[] AS $$
    SELECT CAST(
        CASE
            WHEN i_path LIKE '{%' THEN i_path
            ELSE '{' || REPLACE(i_path, ':', ',') || '}'
        END
    AS text[])
$$ LANGUAGE sql IMMUTABLE;

CREATE OR REPLACE FUNCTION code.easy_get_pillar(
    i_cid   text,
    i_path  text DEFAULT NULL
) RETURNS jsonb AS $$
    SELECT
    CASE
        WHEN i_path IS NOT NULL THEN value #> code.salt2json_path(i_path)
        ELSE value
    END
      FROM dbaas.pillar
     WHERE cid = i_cid;
$$ LANGUAGE sql STABLE;

CREATE OR REPLACE FUNCTION code.easy_update_pillar(
    i_cid       text,
    i_path      text,
    i_val       jsonb,
    i_reason    text DEFAULT NULL,
    i_user_id   text DEFAULT 'admin.local'
) RETURNS void AS $$
DECLARE
    v_cluster record;
BEGIN
    v_cluster := code.lock_cluster(
        i_cid => i_cid,
        i_x_request_id => i_reason
    );

    IF v_cluster.status NOT IN ('RUNNING', 'STOPPED', 'CREATING') THEN
        RAISE EXCEPTION 'Can not update pillar when cluster status is %', v_cluster.status;
    END IF;

    PERFORM code.update_pillar(
        i_cid := i_cid,
        i_rev := v_cluster.rev,
        i_key := code.make_pillar_key(i_cid := i_cid),
        i_value := jsonb_set(
            v_cluster.pillar_value,
            code.salt2json_path(i_path),
            i_val
        )
    );
    PERFORM code.complete_cluster_change(i_cid, v_cluster.rev);
    PERFORM code.add_finished_operation_for_current_rev(
        i_operation_id => gen_random_uuid()::text,
        i_cid => i_cid,
        i_folder_id => v_cluster.folder_id,
        i_operation_type => (v_cluster.type::text || '_modify'),
        i_metadata => '{}'::jsonb,
        i_user_id => i_user_id,
        i_version => 2,
        i_hidden => true,
        i_rev => v_cluster.rev
    );
    RETURN;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION code.easy_delete_pillar(
    i_cid       text,
    i_path      text,
    i_reason    text DEFAULT NULL,
    i_user_id   text DEFAULT 'admin.local'
) RETURNS void AS $$
DECLARE
    v_cluster record;
BEGIN
    v_cluster := code.lock_cluster(
        i_cid => i_cid,
        i_x_request_id => i_reason
    );
    PERFORM code.update_pillar(
        i_cid := i_cid,
        i_rev := v_cluster.rev,
        i_key := code.make_pillar_key(i_cid := i_cid),
        i_value := (v_cluster.pillar_value #- code.salt2json_path(i_path))
    );
    PERFORM code.complete_cluster_change(i_cid, v_cluster.rev);
    PERFORM code.add_finished_operation_for_current_rev(
        i_operation_id => gen_random_uuid()::text,
        i_cid => i_cid,
        i_folder_id => v_cluster.folder_id,
        i_operation_type => (v_cluster.type::text || '_modify'),
        i_metadata => '{}'::jsonb,
        i_user_id => i_user_id,
        i_version => 2,
        i_hidden => true,
        i_rev => v_cluster.rev
    );
    RETURN;
END;
$$ LANGUAGE plpgsql;
