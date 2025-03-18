# proto-file: ci/tasklet/common/proto/service.proto
# proto-message: TaskletContext

job_instance_id {
  job_id: "sawmill-1-1"
  number: 1
}
target_revision {
  hash: "r2"
  number: 2
  pull_request_id: 92

}
secret_uid: "sec-01dy7t26dyht1bj4w3yn94fsa"
release_vsc_info {
}
config_info {
  path: "release/sawmill/a.yaml"
  dir: "release/sawmill"
  id: "simplest-multiply-release"
}
launch_number: 1
flow_triggered_by: "andreevdm"
ci_url: "https://arcanum-test-url/projects/ci/ci/releases/flow?dir=release%2Fsawmill&id=simplest-multiply-release&version=1"
ci_job_url: "https://arcanum-test-url/projects/ci/ci/releases/flow?dir=release%2Fsawmill&id=simplest-multiply-release&version=1&selectedJob=sawmill-1-1&launchNumber=1"
version: "1"
version_info {
    full: "1"
    major: "1"
}
target_commit {
  revision {
    hash: "r2"
    number: 2
    pull_request_id: 92

  }
  date {
    seconds: 1594676509
    nanos: 42000000
  }
  message: "Message"
  author: "sid-hugo"
}
branch: "trunk"
flow_type: DEFAULT
job_triggered_by: "username"
