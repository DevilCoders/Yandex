# proto-file: ci/proto/frontend_project_api.proto
# proto-message: GetConfigStatesResponse

projects {
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
        dir: "autocheck/large-tests"
        title: "Запуск Large и Native тестов"
        flows {
            id {
                dir: "autocheck/large-tests"
                id: "large-flow"
            }
            title: "run-test-flow"
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
}
projects {
    project {
        id: "ci"
        abc_service {
            slug: "ci"
            name {
                ru: "Название"
                en: "CI checks"
            }
            description {
                ru: "Описание"
                en: "Description"
            }
            url: "https://abc.yandex-team.ru/services/ci"
        }
    }
    configs {
        title: "Arcadia CI"
        releases {
            id {
                id: "some-release"
            }
            title: "some-release"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                id: "any-fail-release"
            }
            title: "any-fail-release"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        flows {
            id {
                id: "some-action"
            }
            title: "Some flow"
        }
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "a.yaml"
    }
    configs {
        dir: "a/b/c"
        title: "Woodcutter"
        releases {
            id {
                dir: "a/b/c"
                id: "wood-release"
            }
            title: "Woodcutter"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        flows {
            id {
                dir: "a/b/c"
                id: "sawmill"
            }
            title: "Woodcutter"
        }
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "a/b/c/a.yaml"
    }
    configs {
        dir: "action/flow-vars-ui"
        title: "Woodcutter"
        flows {
            id {
                dir: "action/flow-vars-ui"
                id: "with-flow-vars-ui"
            }
            title: "simple-flow"
        }
        flows {
            id {
                dir: "action/flow-vars-ui"
                id: "with-required-and-default"
            }
            title: "simple-flow"
        }
        flows {
            id {
                dir: "action/flow-vars-ui"
                id: "with-flow-vars-in-title"
            }
            title: "flow-with-var-in-title"
        }
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "action/flow-vars-ui/a.yaml"
    }
    configs {
        dir: "autocheck"
        title: "Woodcutter"
        flows {
            id {
                dir: "autocheck"
                id: "autocheck-branch-precommits"
            }
            title: "Сборка в бранчах r1"
        }
        flows {
            id {
                dir: "autocheck"
                id: "autocheck-trunk-precommits"
            }
            title: "Сборка в транке r1"
        }
        flows {
            id {
                dir: "autocheck"
                id: "autocheck-trunk-precommits-via-testenv"
            }
            title: "Сборка в транке r1"
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
        dir: "ci/missing-task-definition"
        title: "Woodcutter"
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "ci/missing-task-definition/a.yaml"
    }
    configs {
        dir: "ci/no-releases"
        title: "ci without releases"
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "ci/no-releases/a.yaml"
    }
    configs {
        dir: "ci/no/flows"
        title: "Woodcutter"
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "ci/no/flows/a.yaml"
    }
    configs {
        dir: "ci/with-sandbox-template-task"
        title: "Woodcutter"
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "ci/with-sandbox-template-task/a.yaml"
    }
    configs {
        dir: "moved/from"
        title: "Woodcutter"
        flows {
            id {
                dir: "moved/from"
                id: "sawmill"
            }
            title: "Woodcutter"
        }
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "moved/from/a.yaml"
    }
    configs {
        dir: "release-auto/simple"
        title: "Simple Release"
        releases {
            id {
                dir: "release-auto/simple"
                id: "simple"
            }
            title: "Woodcutter"
            auto {
                enabled: true
                editable: true
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "release-auto/simple/a.yaml"
    }
    configs {
        dir: "release/sawmill"
        title: "Yandex CI"
        releases {
            id {
                dir: "release/sawmill"
                id: "demo-sawmill-release"
            }
            title: "Demo samwill release"
            auto {
                # отключено в whiteListOfProcessIds
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
            description: "Demo sawmill description **with some formatting**\nAnd multiline\n"
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "demo-sawmill-no-displacement-release"
            }
            title: "Demo samwill release"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "demo-sawmill-displacement-release"
            }
            title: "Demo samwill release"
            auto {
            }
            release_from_trunk_allowed: true
            release_branches_enabled: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "demo-sawmill-release-conditional"
            }
            title: "Demo samwill release"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "demo-sawmill-release-conditional-vars"
            }
            title: "Demo samwill release"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "empty-release"
            }
            title: "empty-release"
            auto {
            }
            release_from_trunk_allowed: true
            release_branches_enabled: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "empty-manual-release"
            }
            title: "empty-manual-release"
            auto {
            }
            release_from_trunk_allowed: true
            release_branches_enabled: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-release"
            }
            title: "Simplest release process"
            auto {
            }
            release_from_trunk_allowed: true
            release_branches_enabled: true
            branches {
                name: "trunk"
            }
            branches {
                name: "releases/ci/release-component-2"
                created_by: "andreevdm"
                created {
                    seconds: 77880
                }
                base_revision_hash: "r2"
                trunk_commits_count: 1
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-release-with-retry"
            }
            title: "Simplest release process"
            auto {
            }
            release_from_trunk_allowed: true
            release_branches_enabled: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-release-with-start-version"
            }
            title: "Simplest release process"
            auto {
            }
            release_from_trunk_allowed: true
            release_branches_enabled: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-release-override-multiple-resources"
            }
            title: "Simple release process"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-release-override-single-resource"
            }
            title: "Simple release process"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-release-with-manual"
            }
            title: "simplest-release-with-manual"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-sandbox-release"
            }
            title: "Simplest release process"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-sandbox-template-release"
            }
            title: "Simplest release process"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-sandbox-context-release"
            }
            title: "Simplest release process"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-tasklet-v2-release"
            }
            title: "Simplest release process"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-tasklet-v2-simple-release"
            }
            title: "simplest-tasklet-v2-simple-release"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-tasklet-v2-simple-invalid-release"
            }
            title: "simplest-tasklet-v2-simple-invalid-release"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-multiply-release"
            }
            title: "Simplest release process"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-multiply-virtual-release"
            }
            title: "Simplest release process"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-multiply-virtual-release-vars"
            }
            title: "Simplest release process"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-sandbox-multiply-release"
            }
            title: "Simplest release process"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "simplest-sandbox-binary-release"
            }
            title: "Simplest release process"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "tickets-release"
            }
            title: "tickets-release"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "conditional-fail"
            }
            title: "conditional-fail"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/sawmill"
                id: "conditional-fail-with-resources"
            }
            title: "conditional-fail-with-resources"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        flows {
            id {
                dir: "release/sawmill"
                id: "simplest-flow"
            }
            title: "Simplest release process"
        }
        flows {
            id {
                dir: "release/sawmill"
                id: "simplest-action-with-cleanup-without-jobs"
            }
            title: "Simplest flow with manual"
        }
        flows {
            id {
                dir: "release/sawmill"
                id: "simplest-action-with-cleanup"
            }
            title: "Simplest flow with cleanup"
        }
        flows {
            id {
                dir: "release/sawmill"
                id: "simplest-action-with-cleanup-auto"
            }
            title: "Flow из action"
            description: "Flow из action **with some formatting**\nAnd multiline\n"
        }
        flows {
            id {
                dir: "release/sawmill"
                id: "simplest-action-with-cleanup-dependent"
            }
            title: "Simplest flow with cleanup dependent"
        }
        flows {
            id {
                dir: "release/sawmill"
                id: "simplest-action-with-cleanup-fail"
            }
            title: "Simplest flow with cleanup"
        }
        flows {
            id {
                dir: "release/sawmill"
                id: "simplest-action-with-cleanup-fail-success-only"
            }
            title: "Simplest flow with cleanup"
        }
        flows {
            id {
                dir: "release/sawmill"
                id: "simplest-action-with-cleanup-fail-auto"
            }
            title: "Simplest flow with cleanup"
        }
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "release/sawmill/a.yaml"
    }
    configs {
        dir: "valid-at-r1-becomes-invalid-at-r2"
        title: "ci"
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "valid-at-r1-becomes-invalid-at-r2/a.yaml"
    }
    configs {
        dir: "valid-at-r1-becomes-nonci-at-r2"
        title: "ci"
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "valid-at-r1-becomes-nonci-at-r2/a.yaml"
    }
}
projects {
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
}
projects {
    project {
        id: "testenv"
        abc_service {
            slug: "testenv"
            name {
                ru: "Название"
                en: "Testenv"
            }
            description {
                ru: "Описание"
                en: "Description"
            }
            url: "https://abc.yandex-team.ru/services/testenv"
        }
    }
    configs {
        dir: "pr/change"
        title: "Woodcutter"
        flows {
            id {
                dir: "pr/change"
                id: "sawmill"
            }
            title: "Woodcutter"
        }
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "pr/change/a.yaml"
    }
    configs {
        dir: "pr/internal/change"
        title: "Woodcutter"
        flows {
            id {
                dir: "pr/internal/change"
                id: "internal-task-flow"
            }
            title: "Internal flow"
        }
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "pr/internal/change/a.yaml"
    }
    configs {
        dir: "release/action-custom-runtime"
        title: "Simple Release"
        releases {
            id {
                dir: "release/action-custom-runtime"
                id: "my-release"
            }
            title: "my-release"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        flows {
            id {
                dir: "release/action-custom-runtime"
                id: "first"
            }
            title: "sawmill"
        }
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "release/action-custom-runtime/a.yaml"
    }
    configs {
        dir: "release/dummy"
        title: "Simple Release"
        releases {
            id {
                dir: "release/dummy"
                id: "dummy"
            }
            title: "Dummy Release"
            auto {
            }
            release_from_trunk_allowed: true
            release_branches_enabled: true
            branches {
                name: "trunk"
            }
        }
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "release/dummy/a.yaml"
    }
    configs {
        dir: "release/simple-filter"
        title: "Simple Release"
        releases {
            id {
                dir: "release/simple-filter"
                id: "simple"
            }
            title: "Woodcutter"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "release/simple-filter/a.yaml"
    }
    configs {
        dir: "release/simple"
        title: "Simple Release"
        releases {
            id {
                dir: "release/simple"
                id: "simple"
            }
            title: "Woodcutter release"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "release/simple/a.yaml"
    }
    configs {
        dir: "release/with-branches"
        title: "Simple Release"
        releases {
            id {
                dir: "release/with-branches"
                id: "with-branches-enabled-later"
            }
            title: "Woodcutter"
            auto {
            }
            release_from_trunk_allowed: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/with-branches"
                id: "with-branches"
            }
            title: "Woodcutter"
            auto {
            }
            release_from_trunk_allowed: true
            release_branches_enabled: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/with-branches"
                id: "with-branches-forbidden-trunk"
            }
            title: "Woodcutter"
            auto {
            }
            release_branches_enabled: true
            branches {
                name: "trunk"
            }
        }
        releases {
            id {
                dir: "release/with-branches"
                id: "with-auto-branches"
            }
            title: "Woodcutter"
            auto {
            }
            release_from_trunk_allowed: true
            release_branches_enabled: true
            branches {
                name: "trunk"
            }
        }
        created {
            seconds: 1594676509
            nanos: 42000000
        }
        updated {
            seconds: 1594676509
            nanos: 42000000
        }
        path: "release/with-branches/a.yaml"
    }
}
