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
configs {
    dir: "large-test@ci/demo-project/large-tests2"
    title: "Large tests ci/demo-project/large-tests2"
    flows {
        id {
            dir: "large-test@ci/demo-project/large-tests2"
            id: "default-linux-x86_64-release@java"
        }
        title: "default-linux-x86_64-release@java"
    }
    created {
        seconds: 1605098091
    }
    updated {
        seconds: 1605098091
    }
    path: "large-test@ci/demo-project/large-tests2/a.yaml"
    virtual_process_type: VP_LARGE_TESTS
}
xiva_subscription {
    topic: "project@autocheck"
    service: "ci-unit-test"
}
