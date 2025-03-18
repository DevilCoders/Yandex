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
xiva_subscription {
    topic: "project@autocheck"
    service: "ci-unit-test"
}
