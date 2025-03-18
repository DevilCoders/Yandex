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
xiva_subscription {
    topic: "project@serpsearch"
    service: "ci-unit-test"
}
