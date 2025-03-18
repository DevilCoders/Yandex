# proto-file: arcanum/events/arcanum_event.proto
# proto-message: Event

diff_set_is_ready_for_auto_check {
  review_request {
    id: 1785172
    author {
      name: "miroslav2"
    }
    from_branch {
      name: "users/miroslav2/CI-1918-test-cleanup3"
    }
    summary {
      value: "CI-1918: Some test cleanup"
    }
    description {
    }
    repository {
      type: ARC
    }
    to_branch {
      name: "trunk"
    }
    issues {
      id: "CI-1918"
    }
  }
  diff_set {
    iteration_number: 1
    published {
    }
    gsid {
      value: "ARC_MERGE:ds1"
    }
    zipatch {
      svn_base_revision: 8180357
      url: "https://storage-int.mds.yandex.net/get-arcadia-review/1031915/49fbba8b4000a9e9f726e1fd3d1e0911d1b8a4e9/1785172.zipatch"
      human_readable_diff_url: "https://storage-int.mds.yandex.net/get-arcadia-review/1031915/1260263c5c6f97f3bc996b8f4bc1b3bdbbc56ffd/1785172.patch"
    }
    created_at {
      seconds: 1621424707
      nanos: 528603000
    }
    affected_directories {
      paths: "junk/miroslav2/ci/cleanups"
    }
    id: 3736597
  }
  arc_branch_heads {
    from_id: "1c903336ff20422987cc8b727c5a2bc6dca208ad"
    to_id: "r2"
    merge_id: "ds1"
  }
}
