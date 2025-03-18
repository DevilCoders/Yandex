def test_get_commit(curdb):
    COMMIT_ID = 3352614

    repo = curdb.get_repo()

    commit = repo.get_commit(COMMIT_ID)

    assert commit.commit == COMMIT_ID
    assert commit.author == "kimkim"
    assert commit.message == "[manual] Added test group SAS_KIMKIM_TEST."
    assert commit.labels == ["manual"]
    assert set(commit.modified_files) == {"groups/ALL_DYNAMIC/SAS_KIMKIM_TEST.hosts",
                                          "groups/ALL_DYNAMIC/SAS_KIMKIM_TEST.instances",
                                          "groups/ALL_DYNAMIC/card.yaml"}
