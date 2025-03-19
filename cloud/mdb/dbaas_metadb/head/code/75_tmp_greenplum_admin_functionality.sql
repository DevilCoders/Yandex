CREATE OR REPLACE FUNCTION code.random_string(
    i_len integer DEFAULT 17,
    i_prefix text DEFAULt 'mdb'
) RETURNS SETOF TEXT AS
$$
    SELECT (i_prefix || array_to_string(array(SELECT substr('abcdefghijklmnopqrstuvwxyz0123456789', trunc(random() * 36)::integer + 1, 1) FROM generate_series(1, i_len-3)), ''));
$$ LANGUAGE SQL;

CREATE OR REPLACE FUNCTION code.create_greenplum_test_cluster(
    i_name            text,
    i_fqdns_master    text[],         -- {"master.db.yandex.net", "standby.db.yandex.net"}
    i_fqdns_segments  text[],         -- {"seg1.db.yandex.net", "seg2.db.yandex.net"}
    i_env             dbaas.env_type  DEFAULT 'dev'::dbaas.env_type,
    i_network_id      text            DEFAULT '', -- omit this in porto
    i_folder          text            DEFAULT 'mdb-junk',
    i_geo             text            DEFAULT 'iva',
    i_flavor          text            DEFAULT 'db1.nano',
    i_disk_type       text            DEFAULT 'local-ssd',
    i_space_limit     bigint          DEFAULT 21474836480,
    i_request_id      text            DEFAULT 'greenplumhandmade',
    i_subnet_id       text            DEFAULT '', -- omit this in porto
    i_id_prefix      text            DEFAULT 'mdb'
)
RETURNS text
AS $$
DECLARE
    v_pillar              jsonb;
    v_folder              dbaas.folders;
    v_flavor              dbaas.flavors;
    v_cluster             code.cluster_with_labels;
    v_cid                 text = '';
    v_subcid              text = '';
    v_subcid_segs         text = '';
    v_prev_operation      code.operation;
BEGIN

    SELECT * INTO v_flavor FROM dbaas.flavors WHERE name = i_flavor;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'flavor % not found', i_flavor;
    END IF;
    
    SELECT * INTO v_folder FROM dbaas.folders WHERE folder_ext_id = i_folder;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'folder % not found', i_folder;
    END IF;

    v_cid = code.random_string(
        i_prefix => i_id_prefix
    );

    v_subcid = code.random_string(
        i_prefix => i_id_prefix
    );
    v_subcid_segs = code.random_string(
        i_prefix => i_id_prefix
    );
    
    v_cluster = code.create_cluster(
        i_cid          => v_cid,
        i_name         => i_name,
        i_type         => 'greenplum_cluster'::dbaas.cluster_type,
        i_env          => i_env,
        i_public_key   => '',
        i_network_id   => i_network_id,
        i_folder_id    => v_folder.folder_id,
        i_description  => 'greenplum test cluster',
        i_x_request_id => i_request_id
    );

    PERFORM code.add_subcluster(
        i_cid    => v_cid,
        i_subcid => v_subcid,
        i_name   => 'master_subcluster',
        i_roles  => '{greenplum_cluster.master_subcluster}'::dbaas.role_type[],
        i_rev    => v_cluster.rev
    );
    
    PERFORM code.add_subcluster(
        i_cid    => v_cid,
        i_subcid => v_subcid_segs,
        i_name   => 'segment_subcluster',
        i_roles  => '{greenplum_cluster.segment_subcluster}'::dbaas.role_type[],
        i_rev    => v_cluster.rev
    );

    
    v_pillar = ('{"data":{
    "deploy": {
        "version": 2
    },
    "token_service": {
        "address": "ts.private-api.cloud.yandex.net:4282"
    },
    "solomon_cloud": {
        "project": "yandexcloud",
        "service": "mdb_greenplum",
        "ca_path": "/opt/yandex/allCAs.pem",
        "push_url": "https://solomon.cloud.yandex-team.ru/api/v2/push",
        "sa_id": "saidtest",
        "sa_key_id": "sakeyidtest",
        "sa_private_key": "saprivatekey"
    },
    "gp_pkg_version": "6.14.0-11-yandex.51956.62d24f4a45",
    "gp_init": {
        "database_name": "adb",
        "segments_per_disk": 1,
        "mirror_mode": "group"
    },
    "gp_data_folders": [
        "/var/lib/greenplum/data1",
        "/var/lib/greenplum/data2"
    ],
    "gp_admin_prv_key": "-----BEGIN OPENSSH PRIVATE KEY-----\n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\n-----END OPENSSH PRIVATE KEY-----\n",
    "gp_admin_pub_key": "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAACAQDHK5dcCym0MPbny/wyZ8USZsrfHf/pnFtHglrcM2CTNkyouKAiK6VNDwTOeD0A2Dr8qf767lcXxmzHIfhudrpmU+nw6szn10BAnAuiOehFdfrHxNpB0uxMviMlkW0HdryQewe+5Bb5ivcNFqt0atskcPCiwcdFRI9bmn3NUJKCsFff0QTHgqyHcb+IF4EuUV1SwyHaul5/+VZ6e/BpbS9Z/3lcUtLMThKgC4CwkxpJ1Z2Mxj6lgPZTDhmA0sZvDZF+yQppZUt2MfAczldUPhQ9ofPv0cUoSp84JyHYrAFOAjXdcevucpsrlf9t/9jW6Z42KNJyBKtVrsO+pYWxJrBprACxnMa3Y/q6YtwYOrFfarbOCSJAUMEA0aTNi6fDDFo1a4TEutFLOa0fwQeBnfaWnQt5n+JU7THkioskLA7werqPHabRwONhLSl8A5Sc3MnldcMsd6O3DgjsmvPjYhGQ6X7Tdc9Y1Yz61bupwznNhhjkqCxVcWp1BK8Wk+0KvFdd3CmEpW6QPLpGYWSRKjlnkjUOJYVR5RnsbjwyFy1QFatyQH+41gmMVN6INgKf6Xf9+Kv1M5ggLJX0fzGqHjdDRsSfsukFbBdo0ZUX53pIjiE0F86Wr1BQx/D7fs+6qzSwVxUSJQ30iZIU73nnaPrmdnN9E07tSPYFQAHFnyZQ7Q==", 
    "gp_master_directory": "/var/lib/greenplum/data1",
    "standby_install": true,
    "pxf_install": true,
    "pxf_pkg_version": "5.16.1-6-yandex.1054.2bb996ca",
    "greenplum":{
        "config": {
            "optimizer": true,
            "max_connections": {
                "segment": 750,
                "master": 250
            },
            "gp_workfile_limit_per_segment": 0,
            "gp_resource_manager": "queue"
        },
        "users": {
            "gpadmin": {
                "password": "gparray",
                "create": false
            },
            "user1": {
                "password": "user1",
                "login": true,
                "createrole": true,
                "createdb": true,
                "resource_group": "default_group",
                "resource_queue": "pg_default",
                "create": true
            }
        }
    }}}')::jsonb;

    PERFORM code.add_pillar(
        i_cid    => v_cid,
        i_rev    => v_cluster.rev,
        i_value  => v_pillar,
        i_key    => code.make_pillar_key(i_cid => v_cid)
    );

    v_prev_operation = code.add_operation(
        i_operation_id          => code.random_string(17, 'greenplum'),
        i_cid                   => v_cid,
        i_folder_id             => v_folder.folder_id,
        i_time_limit            => '1 hour',
        i_task_type             => 'greenplum_cluster_create',
        i_operation_type        => 'greenplum_cluster_create',
        i_metadata              => '{}'::jsonb,
        i_task_args             => '{}'::jsonb,
        i_user_id               => NULL,
        i_hidden                => false,
        i_version               => 2,
        i_delay_by              => NULL,
        i_required_operation_id => NULL,
        i_idempotence_data      => NULL,
        i_rev                   => v_cluster.rev
    );
    
    SELECT subcid INTO v_subcid FROM dbaas.subclusters WHERE cid = v_cid AND roles = '{greenplum_cluster.master_subcluster}'::dbaas.role_type[]; 
    SELECT subcid INTO v_subcid_segs FROM dbaas.subclusters WHERE cid = v_cid AND roles = '{greenplum_cluster.segment_subcluster}'::dbaas.role_type[];
    
    PERFORM code.update_cloud_usage(
        i_cloud_id   => v_folder.cloud_id,
        i_delta      => code.make_quota(
            i_cpu       => 0,
            i_gpu       => 0,
            i_memory    => 0,
            i_network   => 0,
            i_io        => 0,
            i_ssd_space => 0,
            i_hdd_space => 0,
            i_clusters  => 1
        ),
        i_x_request_id => i_request_id
    );

    FOR i IN 1 .. array_upper(i_fqdns_master, 1)
    LOOP

        PERFORM code.add_host(
            i_subcid           => v_subcid,
            i_shard_id         => NULL::text,
            i_space_limit      => i_space_limit,
            i_flavor_id        => v_flavor.id,
            i_geo              => i_geo,
            i_fqdn             => i_fqdns_master[i], --'greenplum-test02i.db.yandex.net',
            i_disk_type        => i_disk_type,
            i_subnet_id        => i_subnet_id,
            i_assign_public_ip => false,
            i_cid              => v_cid,
            i_rev              => v_cluster.rev
        );
        
        PERFORM code.update_cloud_usage(
            i_cloud_id   => v_folder.cloud_id,
            i_delta      => code.make_quota(
                i_cpu       => v_flavor.cpu_guarantee::real,
                i_gpu       => 0::bigint,
                i_memory    => v_flavor.memory_guarantee::bigint,
                i_network   => v_flavor.network_guarantee::bigint,
                i_io        => v_flavor.io_limit::bigint,
                i_ssd_space => i_space_limit,
                i_hdd_space => 0,
                i_clusters  => 0
            ),
            i_x_request_id => i_request_id
        );
        
        v_prev_operation = code.add_operation(
            i_operation_id          => code.random_string(17, 'greenplum'),
            i_cid                   => v_cid,
            i_folder_id             => v_folder.folder_id,
            i_time_limit            => '1 hour',
            i_task_type             => 'greenplum_cluster_create',
            i_operation_type        => 'greenplum_host_create',
            i_metadata              => '{}'::jsonb,
            i_task_args             => '{}'::jsonb,
            i_user_id               => NULL,
            i_hidden                => false,
            i_version               => 2,
            i_delay_by              => NULL,
            i_required_operation_id => v_prev_operation.operation_id,
            i_idempotence_data      => NULL,
            i_rev                   => v_cluster.rev
        );

    END LOOP;
    
    FOR i IN 1 .. array_upper(i_fqdns_segments, 1)
    LOOP

        PERFORM code.add_host(
            i_subcid           => v_subcid_segs,
            i_shard_id         => NULL::text,
            i_space_limit      => i_space_limit,
            i_flavor_id        => v_flavor.id,
            i_geo              => i_geo,
            i_fqdn             => i_fqdns_segments[i], --'greenplum-test06i.db.yandex.net',
            i_disk_type        => i_disk_type,
            i_subnet_id        => i_subnet_id,
            i_assign_public_ip => false,
            i_cid              => v_cid,
            i_rev              => v_cluster.rev
        );
        
        PERFORM code.update_cloud_usage(
            i_cloud_id   => v_folder.cloud_id,
            i_delta      => code.make_quota(
                i_cpu       => v_flavor.cpu_guarantee::real,
                i_gpu       => 0::bigint,
                i_memory    => v_flavor.memory_guarantee::bigint,
                i_network   => v_flavor.network_guarantee::bigint,
                i_io        => v_flavor.io_limit::bigint,
                i_ssd_space => i_space_limit,
                i_hdd_space => 0,
                i_clusters  => 0
            ),
            i_x_request_id => i_request_id
        );
        
        v_prev_operation = code.add_operation(
            i_operation_id          => code.random_string(17, 'greenplum'),
            i_cid                   => v_cid,
            i_folder_id             => v_folder.folder_id,
            i_time_limit            => '1 hour',
            i_task_type             => 'greenplum_cluster_create',
            i_operation_type        => 'greenplum_host_create',
            i_metadata              => '{}'::jsonb,
            i_task_args             => '{}'::jsonb,
            i_user_id               => NULL,
            i_hidden                => false,
            i_version               => 2,
            i_delay_by              => NULL,
            i_required_operation_id => v_prev_operation.operation_id,
            i_idempotence_data      => NULL,
            i_rev                   => v_cluster.rev
        );

    END LOOP;

    PERFORM code.complete_cluster_change(v_cid, v_cluster.rev);
   
    RETURN v_cid;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION code.insert_sample_cluster(
    i_folder          text            DEFAULT 'mdb-junk',
    i_flavor          text            DEFAULT 's1.compute.1',
    i_disk_type       text            DEFAULT 'local-ssd'
)
RETURNS void
AS $$
DECLARE
BEGIN

    PERFORM code.create_greenplum_test_cluster(
        i_name           => 'greenplum_test_cluster_13337'::text,
        i_fqdns_master   => '{"greenplum-test02i.db.yandex.net"}'::text[],
        i_fqdns_segments => '{"greenplum-test04i.db.yandex.net"}'::text[],
        i_folder         => i_folder,
        i_disk_type      => i_disk_type,
        i_flavor         => i_flavor
    ); --create sample cluster
    
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION code.delete_greenplum_test_cluster(
    i_cid           text,
    i_request_id    text            DEFAULT 'greenplumhandmade'
)
RETURNS void
AS $$
DECLARE
    v_cluster     code.cluster_with_labels;
    v_del_op      code.operation;
    v_del_meta_op code.operation;
BEGIN
    v_cluster = code.lock_cluster(i_cid => i_cid, i_x_request_id => i_request_id);
    IF v_cluster IS NULL THEN
        RAISE EXCEPTION 'cluster % not found', i_cid;
    END IF;
    IF v_cluster.type::text != 'greenplum_cluster' THEN
        RAISE EXCEPTION 'cluster % is not greenplum', i_cid;
    END IF;
    
    v_del_op = code.add_operation(
        i_operation_id          => code.random_string(17, 'greenplum'),
        i_cid                   => v_cluster.cid,
        i_folder_id             => v_cluster.folder_id,
        i_time_limit            => '1 hour',
        i_task_type             => 'greenplum_cluster_delete',
        i_operation_type        => 'greenplum_cluster_delete',
        i_metadata              => '{}'::jsonb,
        i_task_args             => ('{}')::jsonb,
        i_user_id               => NULL,
        i_hidden                => false,
        i_version               => 2,
        i_delay_by              => NULL,
        i_required_operation_id => NULL,
        i_idempotence_data      => NULL,
        i_rev                   => v_cluster.rev
    );
    
    v_del_meta_op = code.add_operation(
        i_operation_id          => code.random_string(17, 'greenplum'),
        i_cid                   => v_cluster.cid,
        i_folder_id             => v_cluster.folder_id,
        i_time_limit            => '1 hour',
        i_task_type             => 'greenplum_cluster_delete_metadata',
        i_operation_type        => 'greenplum_cluster_delete',
        i_metadata              => '{}'::jsonb,
        i_task_args             => ('{}')::jsonb,
        i_user_id               => NULL,
        i_hidden                => false,
        i_version               => 2,
        i_delay_by              => '10 minutes'::interval,
        i_required_operation_id => v_del_op.operation_id,
        i_idempotence_data      => NULL,
        i_rev                   => v_cluster.rev
    );

    PERFORM code.add_operation(
        i_operation_id          => code.random_string(17, 'greenplum'),
        i_cid                   => v_cluster.cid,
        i_folder_id             => v_cluster.folder_id,
        i_time_limit            => '1 hour',
        i_task_type             => 'greenplum_cluster_purge',
        i_operation_type        => 'greenplum_cluster_delete',
        i_metadata              => '{}'::jsonb,
        i_task_args             => ('{}')::jsonb,
        i_user_id               => NULL,
        i_hidden                => false,
        i_version               => 2,
        i_delay_by              => '10 minutes'::interval,
        i_required_operation_id => v_del_meta_op.operation_id,
        i_idempotence_data      => NULL,
        i_rev                   => v_cluster.rev
    );
    PERFORM code.complete_cluster_change(v_cluster.cid, v_cluster.rev);
END;
$$ LANGUAGE plpgsql;

