Feature: katan.clusters

  Background:
    Given database at last migration

  Scenario: Cluster tags should be an object
    When I execute script
    """
    INSERT INTO katan.clusters (cluster_id, tags) VALUES ('cid1', '[]');
    """
    Then it fail with error "new row for relation "clusters" violates check constraint "check_tags_contain_source_and_be_an_object"
