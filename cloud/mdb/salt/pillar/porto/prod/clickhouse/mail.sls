data:
    clickhouse:
        users:
            mail:
                password: {{ salt.yav.get('ver-01e0wttxckw3k8xzhgfgfhk1my[password]') }}
                hash: {{ salt.yav.get('ver-01e0wttxckw3k8xzhgfgfhk1my[hash]') }}
                profile: default
                databases:
                    - mail
                    - mailstatsdb
                    - sender
            reader:
                password: {{ salt.yav.get('ver-01e0wtx1wnq3xvy0gjgkrh5rew[password]') }}
                hash: {{ salt.yav.get('ver-01e0wtx1wnq3xvy0gjgkrh5rew[hash]') }}
                profile: readonly
                databases:
                    - mail
                    - mailstatsdb
                    - sender
