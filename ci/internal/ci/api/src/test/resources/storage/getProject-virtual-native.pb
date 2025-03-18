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
    dir: "native-build@ci/demo-project/large-tests2"
    title: "Native builds ci/demo-project/large-tests2"
    flows {
        id {
            dir: "native-build@ci/demo-project/large-tests2"
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
    path: "native-build@ci/demo-project/large-tests2/a.yaml"
    virtual_process_type: VP_NATIVE_BUILDS
}
xiva_subscription {
    topic: "project@autocheck"
    service: "ci-unit-test"
}
