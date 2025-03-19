{%- set env = grains['yandex-environment'] -%}
{%- if env == 'testing' -%}
{%- set nscfg_host = 'nscfg.mdst.yandex.net' -%}
{%- set banned_urls_host = 's3.mdst.yandex.net' -%}
{%- set cluster_prefix = 'testing_srw' -%}
{%- set runlist = 'zerro' -%}
{%- set tvm_client_id = 2000272 -%}
{%- set pg_table_name = 'cocaine_index_testing' -%}
{%- set pg_host = 'c-dce9af9b-999a-4c0a-87f7-3a66aeacfb59.rw.db.yandex.net' -%}
{%- set elliptics_core_nodes = ['elisto01i.tst.ape.yandex.net:1025:10', 'elisto01f.tst.ape.yandex.net:1025:10'] -%}
{%- set elliptics_core_groups = [3, 4] -%}
{%- set elliptics_core_copies = 'any' -%}
{%- set pg_dbname = 'ape_test' -%}
{%- set pg_dbuser = 'ape_test' -%}
{%- set unicorns = {
        "core": {
            "type": "zookeeper",
            "args": {
                "endpoints": [
                    {"host":"zk01h.tst.ape.yandex.net", "port": 2181},
                    {"host":"zk01i.tst.ape.yandex.net", "port": 2181},
                    {"host":"zk01f.tst.ape.yandex.net", "port": 2181}
                ],
                "prefix": "/ape-test",
                "recv_timeout_ms": 5000
            }
        },
        "dark": {
            "type": "zookeeper",
            "args": {
                "endpoints": [
                    {"host":"zk01h.tst.ape.yandex.net", "port": 2181},
                    {"host":"zk01i.tst.ape.yandex.net", "port": 2181},
                    {"host":"zk01f.tst.ape.yandex.net", "port": 2181}
                ],
                "prefix": "/ape-test-dark",
                "recv_timeout_ms": 5000
            }
        },
        "discovery": {
            "type": "zookeeper",
            "args": {
                "endpoints": [
                    {"host":"zk01h.tst.ape.yandex.net", "port": 2181},
                    {"host":"zk01i.tst.ape.yandex.net", "port": 2181},
                    {"host":"zk01f.tst.ape.yandex.net", "port": 2181}
                 ],
                "prefix": "/ape-test-discovery",
                "recv_timeout_ms": 5000
            }
        }
} -%}
{%- set mastermind_remotes = [['[::]', 8383]] -%}
{%- set mg_base_uri = "http://storage.stm.yandex.net:10010" -%}
{%- set mg_read_uri = "/gate/get/" -%}
{%- else -%}
{%- set cluster_prefix = 'production_mds' -%}
{%- set nscfg_host = 'nscfg.mds.yandex.net' -%}
{%- set banned_urls_host = 's3.mds.yandex.net' -%}
{%- set runlist = 'zerro' -%}
{%- set tvm_client_id = 2000273 -%}
{%- set pg_table_name = 'cocaine_index_prod' -%}
{%- set pg_dbname = 'apedb' -%}
{%- set pg_dbuser = 'ape' -%}
{%- set pg_host = 'c-3d57537b-1380-4b16-9358-54c3e6f14a6b.rw.db.yandex.net' -%}
{%- set elliptics_core_nodes = ['elisto01h.ape.yandex.net:1025:10', 'elisto02h.ape.yandex.net:1025:10', 'elisto03h.ape.yandex.net:1025:10', 'elisto04e.ape.yandex.net:1025:10', 'elisto05e.ape.yandex.net:1025:10', 'elisto06e.ape.yandex.net:1025:10'] -%}
{%- set elliptics_core_groups = [1, 4] -%}
{%- set elliptics_core_copies = 'quorum' -%}
{%- set unicorns = {
        "core": {
            "type": "zookeeper",
            "args": {
                "endpoints": [
                    {"host":"zk03f.ape.yandex.net", "port": 2181},
                    {"host":"zk03i.ape.yandex.net", "port": 2181},
                    {"host":"zk03h.ape.yandex.net", "port": 2181}
                ],
                "prefix": "/ape",
                "recv_timeout_ms": 5000
            }
        },
        "dark": {
            "type": "zookeeper",
            "args": {
                "endpoints": [
                    {"host":"zk05f.ape.yandex.net", "port": 2181},
                    {"host":"zk05i.ape.yandex.net", "port": 2181},
                    {"host":"zk05h.ape.yandex.net", "port": 2181}
                ],
                "prefix": "/ape_dark",
                "recv_timeout_ms": 5000
            }
        },
        "discovery": {
            "type": "zookeeper",
            "args": {
                "endpoints": [
                    {"host":"zk04i.ape.yandex.net", "port": 2181},
                    {"host":"zk04h.ape.yandex.net", "port": 2181},
                    {"host":"zk04f.ape.yandex.net", "port": 2181}
                ],
                "prefix": "/ape-discovery",
                "recv_timeout_ms": 5000
            }
        }
} -%}
{%- set mastermind_remotes = [["[::]", 8383]] -%}
{%- set mg_base_uri = "http://storage.mail.yandex.net:8080" -%}
{%- set mg_read_uri = "/gate/get/tvm/" -%}
{%- endif -%}

{
    "version": 4,
    "logging": {
        "loggers" : {
            "core": [
                {
                    "formatter": {
                        "type": "tskv",
                        "create": {
                            "tskv_format": "cocaine-log",
                            "type": "cocaine-backend"
                        },
                        "mutate": {
                            "unixtime_microsec_utc": {"strftime": "%s.%f", "gmtime": false},
                            "timestamp": {"strftime": "%Y-%m-%dT%H:%M:%S"},
                            "timezone": {"strftime": "%z"}
                        }
                    },
                    "sinks": [
                        {
                            "type" : "asynchronous",
                            "sink": {
                                "type": "file",
                                "path": "/var/log/cocaine-core/cocaine-tskv.log",
                                "flush": 100
                             },
                             "overflow": "drop",
                             "factor": 20
                        }
                    ]
                }
            ],
            "logsrv": [
                {
                    "formatter": {
                        "type": "tskv",
                        "create": {
                            "tskv_format": "cocaine-log",
                            "type": "cocaine-backend"
                        },
                        "mutate": {
                            "unixtime_microsec_utc": {"strftime": "%s.%f", "gmtime": false},
                            "timestamp": {"strftime": "%Y-%m-%dT%H:%M:%S"},
                            "timezone": {"strftime": "%z"}
                        }
                    },
                    "sinks": [
                        {
                            "type" : "asynchronous",
                            "sink": {
                                "type": "file",
                                "path": "/var/log/cocaine-core/cocaine-logging-tskv.log",
                                "flush": 100
                             },
                             "overflow": "drop",
                             "factor": 20
                        }
                    ]
                }
            ],
        },
        "severity": "info"
    },

    "network": {
        "pinned": {
            "locator": 10071,
            "node": 10072,
            "storage": 10073,
            "logging": 10075,
            "unicorn": 10076,
            "unistorage": 10077,
            "tvm2": 10078
        },
        "shared":[32223,32723]
    },

    "authorizations": {
        "event": {
            "type": "disabled",
            "args": {
                "unicorn": "core"
            }
        },

        "storage": {
            "type": "disabled",
            "args": {
                "backend": "core"
            }
        },

        "unicorn": {
            "type": "disabled",
            "args": {
                "backend": "core"
            }
        }
    },

    "paths": {
        "plugins": [
            "/usr/lib/cocaine"
        ],
        "runtime": "/var/run/cocaine"
    },

    "services": {
        "logging": {
            "type": "logging::v2",
            "args": {
                "backend": "logsrv",
                "default_metafilter": [
                    ["&&",
                        ["severity", 1],
                        ["||",
                            ["!=", "source", "unistorage/mds/elliptics/client"],
                            ["&&",
                                ["==", "source", "unistorage/mds/elliptics/client"],
                                ["severity", 3]
                            ]
                        ]
                    ],
                    ["traced"]
                ],
                "truncate": {
                    "message_size": 50000
                }
            }
        },

        "metrics": {
            "type": "metrics"
        },

        "tvm2": {
            "type": "tvm2",
            "args": {
                "base_url": "https://tvm-api.yandex.net",
                "client_id": 92,
                "client_secret": "{{ pillar['yav']['tvm_92_client_secret'] }}"
            }
        },

        "unistorage": {
            "type": "unistorage",
            "args": {
{% if env in 'testing' %}
                "authentication_backend": "mds",
{% endif %}
                "backends" : ["disk-singular-zero", "libmds", "mulcagate"],
                "keyring_path": "/etc/cocaine/auth.keys"
            }
        },

        "storage": {
            "type": "storage"
        },

        "locator": {
            "type": "locator",
            "args": {
                "cluster": {
                    "type": "unicorn",
                    "args": {
                        "backend": "discovery"
                    }
                },
                "extra_param":{
                    {# MDS-19540 -#}
                    {% if grains['conductor']['root_datacenter'] == 'vlx' -%}
                    "x-cocaine-cluster": "{{ cluster_prefix }}_vla"
                    {%- else -%}
                    "x-cocaine-cluster": "{{ cluster_prefix }}_{{ grains['conductor']['root_datacenter'] }}"
                    {%- endif %}
                }
            },
            "restrict": ["logging"],
            "restrict": ["unicorn"]
        },

        "uniresis": {
            "type": "uniresis",
            "args": {
                "prefix": "/darkvoice/nodes"
            }
        },

         "unicorn": {
           "type": "unicorn",
           "args": {
             "backend": "core"
           }
        },

        "unicorn_dark": {
           "type": "unicorn",
           "args": {
             "backend": "dark"
           }
        },

        "node": {
            "type": "node::v2",
            "args": {
                "runlist": "{{ runlist }}"
            }
        }
    },
    "storages": {
        "core": {
                "type" : "postgres",
                "args" : {
                    "pg_backend" : "core",
                    "pg_table_name" : "{{ pg_table_name }}",
                    "pg_underlying_storage" : "elliptics_core",
                    "pg_key_column" : "key",
                    "pg_collection_column" : "collection",
                    "pg_tags_column" : "tags"
                }
        }, 
        "elliptics_core": {
            "type": "elliptics",
            "args": {
                "nodes" : {{ elliptics_core_nodes | json }},
                "io-thread-num" : 2,
                "wait-timeout" : 30,
                "check-timeout" : 60,
                "net-thread-num" : 2,
                "groups" : {{ elliptics_core_groups }},
                "timeouts" : {
                    "read" : 60,
                    "write" : 60,
                    "remove" : 60,
                    "find" : 60
                },
                "success-copies-num" : "{{ elliptics_core_copies }}",
                "flags" : 4,
		"read_latest": true,
                "verbosity" : 4
            }
        }
    },

    "authentications": {
{% if env in 'testing' %}
        "mds": {
            "type": "tvm",
            "args": {
                "allow_anonymous": true,
                "client_id": 2000272,
                "client_secret": "{{ pillar['yav']['storage-testing_tvm_secret'] }}",
                "secure_storage": "core",
                "base_url": "https://tvm-api.yandex.net"
            }
        },
{% endif %}
        "core": {
            "type": "tvm",
            "args": {
                "allow_anonymous": true,
                "client_id": 92,
                "client_secret": "{{ pillar['yav']['tvm_92_client_secret'] }}",
                "secure_storage": "core",
                "base_url": "https://tvm-api.yandex.net",
                "keyring_path": "/etc/cocaine/auth.keys"
                }
        }
   },

    "postgres": {
        "core": {
            "args" : {
                "pool_size" : 3,
                "connection_string" : "dbname={{ pg_dbname }} user={{ pg_dbuser }} port=6432 host={{ pg_host }} sslmode=verify-full sslrootcert=/usr/share/yandex-internal-root-ca/YandexInternalRootCA.crt"
            }
        }
    },

    "unicorns": {{ unicorns | json }},

    "unistorage_backends": {
        "disk-singular-zero": {
            "type": "disk-singular-zero"
        },
        "libmds": {
            "type": "libmds",
            "args": {
                "adapter_verbosity": 0,
                "data_flow_rate": 10,
                "default_chunk_size": 16777216,
                "debug_mode": false,
                "elliptics_remote_port": 1025,
                "check_timeout": 3,
                "lookup_timeout": 15,
                "read_timeout": 20,
                "wait_timeout": 5,
                "recovery_timeout": 20,
                "mastermind_remotes" : {{ mastermind_remotes | json }},
                "mastermind_app_name" : "mastermind-drooz-http",
                "mastermind_cache_path": "/var/cache/mastermind/unistorage/unistorage.cache",
                "mastermind_update_period": 60,
                "mastermind_warning_time": 3600,
                "mastermind_expire_time": 86400000,
                "mastermind_enqueue_timeout_ms": 50000,
                "mastermind_reconnect_timeout_ms": 5000,
                "maximum_chunk_size": 67108864,
                "minimal_chunk_size": 1048576,
                "use_elliptics_proxy": true,
                {%- if 'lepton' in config %}
                "enable_lepton": {{config.lepton.enable | json}},
                "enable_lepton_comparison": {{config.lepton.comparison | json}},
                "lepton_address": {{config.lepton.address | json}},
                "local_lepton_address": {{config.lepton.local_address | default("http://[::1]:7070/") | json}},
                {%- endif %}
                "acl": {
                    "enable": true,
                    "http_source": "http://{{ banned_urls_host }}/mds-service/banned_urls",
                    "cache_path": "/var/cache/cocaine/cache",
                    "update_interval_sec": 60,
                    "http_request_timeout_msec": 2000
                },
                {%- if config.mavrodi.enabled %}
                "meta-mastermind" : {
                        "max-history-depth" : 2,
                        "max-snapshot-age-sec": 86400,
                        "snapshots-dir" : "/var/cache/mastermind/unistorage",
                        "grpc-endpoints": [
                            {%- for r in config.mavrodi.remotes %}
                            "{{ r }}:{{ config.mavrodi.port }}"{{ "," if not loop.last else "" }}
                            {%- endfor %}
                        ],
                        "grpc-calls-before-endpoint-switch" : 6,
                        "grpc-call-timeout-sec" : 20,
                        "snapshot-update-interval-sec" : 60,
                        "tls": {
                         "enabled": true,
                         "cert_path": "/etc/mavrodi/ssl/mavrodi.crt",
                         "key_path": "/etc/mavrodi/ssl/mavrodi.key",
                         "ca_paths": ["/etc/mavrodi/ssl/ca.crt"]
                       }
                },
                {%- endif %}
                "karl": {
                   "enabled": {{ config.karl.enabled|lower }},
                   "use_federations": true,
                   "use_locality": true,
                   "default_federation": 1,
                   "discovery": {
                       "connection_string": "{{ config.karl.connection_string }}",
                       "prefix": "{{ config.karl.prefix }}",
                       "snapshot_filename" : "/var/cache/cocaine/karl_discovery.snapshot"
                   },
                    "tls": {
                       "enabled": true,
                       "cert_path": "/etc/karl/ssl/karl.crt",
                       "key_path": "/etc/karl/ssl/karl.key",
                       "ca_paths": ["/etc/karl/ssl/ca.crt"]
                   }
                },
                "nscfg": {
                    "enabled": true,
                    "http_source": "http://{{ nscfg_host }}:9532/all",
                    "warning_time_sec": 1200,
                    "cache_path": "/var/cache/cocaine/nscfg-settings.cache",
                    "update_interval_sec": 60,
                    "http_request_timeout_msec": 10000
                },
                "tls": {
                    "support": 1,
                    "cert_path": "/etc/elliptics/ssl/storage.crt",
                    "key_path": "/etc/elliptics/ssl/storage.key",
                    "ca_path": "/etc/elliptics/ssl/"
                },
                "elliptics_features": {
                    "independent_protocol": true
                },
                "auth": {
                    "enabled": true,
                    "tvm": {
                        "client_id": {{ tvm_client_id }},
                        "keys_cache_dir": "/var/cache/cocaine/tvm"
                    }
                },
                "handystats": {
                    "enable": true,
                    "statistics": {
                        "moving-interval": 5000,
                        "histogram-bins": 30,
                        "tags": ["moving-avg", "histogram"]
                    },
                    "gauge": {
                        "tags": ["histogram"]
                    },
                    "counter": {
                        "tags": ["value"]
                    },
                    "timer": {
                        "idle-timeout": 60000,
                        "tags": ["moving-avg"]
                    }
                }
            }
        },
        "mulcagate": {
             "type":"mulcagate",
             "args":{
                "base_uri" : "{{ mg_base_uri }}",
                "read_uri" : "{{ mg_read_uri }}",
                "read_timeout_ms" : 5000000,
                "size_header" : "X-Data-Size"
            }
        }
    }
}

