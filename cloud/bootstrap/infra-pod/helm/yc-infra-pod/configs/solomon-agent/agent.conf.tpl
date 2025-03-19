{{- with .Values.solomonAgent }}
Logger {
    LogTo: STDERR
    Level: INFO
}

Python2 {
    # defines should agent try to load/write .pyc files (default: false)
    IgnorePycFiles: true
}

HttpServer {
    BindPort: {{ required "You must specify bind port for http server" .httpServer.bindPort }}
    MaxConnections: 100
    OutputBufferSize: 256
    MaxQueueSize: 200
    ThreadPoolName: "Io"
}

Storage {
    Limit: {
        Total: "2GiB"
        ShardDefault: "100MiB"
    }
}

ConfigLoader {
    {{- if .services }}
    Static {
        {{- range .services }}
        Services {
{{ . | indent 12 }}
        }
        {{- end }}
    }
    {{- end }}

    {{- if .ycSolomon }}
    Python2Loader {
        UpdateInterval: "60s",
        FilePath: "/usr/lib/python2.7/dist-packages/yc_solomon_plugins/common.py",
        ModuleName: "common",
        ClassName: "CommonLoader"

        Params {
          key: "plugins"
          value: "{{ .ycSolomon.plugins| join "," }}"
        }

        {{- if .ycSolomon.params }}
        {{- range .ycSolomon.params }}
        Params {
          key: {{ .key }}
          value: {{ .value }}
        }
        {{- end }}
        {{- end }}
    }
    {{- end }}
}

{{- if .pushConf.enabled }}
{{- with .pushConf }}
Push {
    {{- if .endpoint }}
    {{- with .endpoint }}
    Endpoints: [
        {
             Type: "{{ .type }}"
             IamConfig {
                ServiceAccountId: "{{ .serviceAccountId }}"
                KeyId: "{{ .keyId }}"
                PublicKeyFile: "{{ .publicKey }}"
                PrivateKeyFile: "{{ .privateKey }}"
             }
        }
    ]
    {{- end }}
    {{- end }}
    {{- if .hosts }}
    {{- with .hosts }}
    Hosts: [
        {
             Url: "{{ .url }}"
        }
    ],
    {{- end }}
    {{- end }}

  {{- if .shards  }}
    Shards: [
    {{- range .shards }}
        {
            Project: "{{ .project }}"
            Service: "{{ .service }}"
        },
    {{- end }}
    ]
  {{- else -}}
    AllShards: true
  {{- end }}

    Cluster: "default"

    PushInterval: "30s"
    RetryInterval: "5s"
    RetryTimes: 10
    ShardKeyOverride {
        Project: "{{ "{{cloud_id}}" }}"
        Cluster: "{{ "{{folder_id}}" }}"
        Service: "{{ .service }}"
    }
}
{{- end }}
{{- end }}

# management API server settings
ManagementServer {
    BindAddress: "::"
    BindPort: {{ required "You must specify bind port for management server" .managementServer.bindPort }}
    ThreadPoolName: "Io"
}

ThreadPoolProvider {
  ThreadPools: [
    {
      Name: "Io"
      Threads: 4
    },
    {
      Name: "Default"
      Threads: 8
    }
  ]
}

{{- if .httpPush.enabled }}
Modules {
    HttpPush {
        BindAddress: "::1"
        BindPort: {{ required "You must specify bind port for http push" .httpPush.bindPort }}
        Name: "httpPush"

        Handlers [
        {{- range .httpPush.handlers }}
            Endpoint: "{{ .endpoint }}"
            Project: "{{ .project }}"
            Service: "{{ .service }}"
        {{- end }}
        ]
    }
}
{{- end }}
{{- end }}
