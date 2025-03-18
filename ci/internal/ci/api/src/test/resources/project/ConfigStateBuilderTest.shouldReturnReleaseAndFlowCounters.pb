# proto-file: ci/proto/common.proto
# proto-message: ConfigState

dir: "/my/path"
title: "config title"
releases {
    id {
        dir: "/my/path"
        id: "flow"
    }
    title: "release title"
    auto {
    }
    release_from_trunk_allowed: true
    release_branches_enabled: true
    branches {
        name: "trunk"
        launch_status_counter {
            status: RUNNING
            count: 2
        }
        launch_status_counter {
            status: FAILURE
            count: 1
        }
    }
    branches {
        name: "releases/ci/1"
        created_by: "login"
        created {
            seconds: 1609459200
        }
        base_revision_hash: "r1"
        trunk_commits_count: -1
        branch_commits_count: -1
        launch_status_counter {
            status: RUNNING
            count: 1
        }
    }
}
flows {
    id {
        dir: "/my/path"
        id: "flow"
    }
    title: "flow title"
}
created {
}
updated {
}
path: "/my/path/a.yaml"
