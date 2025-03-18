# proto-file: ci/proto/frontend_project_api.proto
# proto-message: GetProjectResponse

project {
    id: "serpsearch"
    abc_service {
        slug: "serpsearch"
        name {
            ru: "Название"
            en: "Выдача поиска (SERP)"
        }
        description {
            ru: "Описание"
            en: "Description"
        }
        url: "https://abc.yandex-team.ru/services/serpsearch"
    }
}
configs {
    dir: "not/ci"
    title: "Search Engine Results Page (web4)"
    created {
        seconds: 1594676509
        nanos: 42000000
    }
    updated {
        seconds: 1594676509
        nanos: 42000000
    }
    path: "not/ci/a.yaml"
}
configs {
    dir: "not/ci/modified"
    title: "Search Engine Results Page (web4)"
    created {
        seconds: 1594676509
        nanos: 42000000
    }
    updated {
        seconds: 1594676509
        nanos: 42000000
    }
    path: "not/ci/modified/a.yaml"
}
configs {
    dir: "not/ci/not-modified"
    title: "Search Engine Results Page (web4)"
    created {
        seconds: 1594676509
        nanos: 42000000
    }
    updated {
        seconds: 1594676509
        nanos: 42000000
    }
    path: "not/ci/not-modified/a.yaml"
}
xiva_subscription {
    topic: "project@serpsearch"
    service: "ci-unit-test"
}
