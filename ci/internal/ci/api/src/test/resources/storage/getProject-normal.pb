# proto-file: ci/proto/frontend_project_api.proto
# proto-message: GetProjectResponse

project {
    id: "autocheck"
    abc_service {
        slug: "autocheck"
        name {
            ru: "Название"
            en: "Автосборка"
        }
        description {
            ru: "Описание"
            en: "Description"
        }
        url: "https://abc.yandex-team.ru/services/autocheck"
    }
}
configs {
    dir: "autocheck"
    title: "Запуск Large и Native тестов"
    flows {
        id {
            dir: "autocheck"
            id: "run-flow"
        }
        title: "Run tests"
    }
    created {
        seconds: 1594676509
        nanos: 42000000
    }
    updated {
        seconds: 1594676509
        nanos: 42000000
    }
    path: "autocheck/a.yaml"
}
configs {
    dir: "autocheck/large-tests"
    title: "Запуск Large и Native тестов"
    flows {
        id {
            dir: "autocheck/large-tests"
            id: "large-flow"
        }
        title: "Run tests"
    }
    created {
        seconds: 1594676509
        nanos: 42000000
    }
    updated {
        seconds: 1594676509
        nanos: 42000000
    }
    path: "autocheck/large-tests/a.yaml"
}
xiva_subscription {
    topic: "project@autocheck"
    service: "ci-unit-test"
}
