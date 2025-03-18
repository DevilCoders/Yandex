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
      name: "releases/experimental/mobile/metrika/ios/appmetrica-sdk/4.0.0"
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
      value: "ARC_MERGE:e9e1c7501268367aba7b61f2f484927abe6e5595"
    }
    zipatch {
      svn_base_revision: 8180346
      url: "https://storage-int.mds.yandex.net/get-arcadia-review/1031915/c46bddab266a7186b4940fe0084bd622768633e1/1785172.zipatch"
      human_readable_diff_url: "https://storage-int.mds.yandex.net/get-arcadia-review/860113/5283ed23073080d08f010a4fc18a1e2c1c58fefc/1785172.patch"
    }
    created_at {
      seconds: 1621424600
      nanos: 217828000
    }
    affected_directories {
      paths: "junk/miroslav2/ci/cleanups"
    }
    id: 3736581
  }
  arc_branch_heads {
    from_id: "460696d2531ca9c292cbb2eea1c7af66a582bde9"
    to_id: "49117bd16acce6099f4739a3a293736e7cece99f"
    merge_id: "e9e1c7501268367aba7b61f2f484927abe6e5595"
  }
}
