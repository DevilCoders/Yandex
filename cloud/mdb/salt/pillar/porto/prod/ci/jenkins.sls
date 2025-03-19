data:
    tvmtool:
        config:
            client: mdb-jenkins-sso
            secret: {{ salt.yav.get('ver-01e0cqgrabvfyzky04cqtftfp7[secret]') }}
        port: 50001
        token: {{ salt.yav.get('ver-01e0cqgrabvfyzky04cqtftfp7[token]') }}
        tvm_id: 2011258
    jenkins:
        server_name: jenkins.db.yandex-team.ru
