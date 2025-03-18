OWNER(g:ci)

DOCS(docs)
DOCS_INCLUDE_SOURCES(
    ci/internal/ci/core/src/test/resources/ayaml/release-start-version.yaml
    ci/internal/ci/core/src/test/resources/ayaml/example-in-docs.yaml
    ci/internal/ci/core/src/test/resources/ayaml/runtime/sandbox-notifications.yaml
    ci/internal/ci/core/src/test/resources/ayaml/sandbox-requirements.yaml
    ci/internal/ci/core/src/test/resources/ayaml/release-auto.yaml
    ci/internal/ci/core/src/test/resources/ayaml/job-attempts/valid.yaml
    ci/internal/ci/core/src/test/resources/ayaml/job-attempts/valid-conditional.yaml
    ci/internal/ci/core/src/test/resources/ayaml/job-attempts/valid-reuse.yaml
    ci/internal/ci/core/src/test/resources/ayaml/job-attempts/valid-tasklet-v2.yaml
    ci/internal/ci/core/src/test/resources/ayaml/flow-vars-ui/valid.yaml
    ci/internal/ci/core/src/test/resources/ayaml/flow-vars-ui/declared-both.yaml
    ci/internal/ci/core/src/test/resources/ayaml/with-runtime.yaml
    ci/internal/ci/core/src/test/resources/ayaml/with-requirements.yaml
    ci/internal/ci/core/src/test/resources/task/task-with-runtime.yaml
    ci/internal/ci/core/src/test/resources/task/task-with-requirements.yaml
)

DOCS_BUILDER(yfm)

END()
