Feature: Lint success

  Scenario: Lint code schema
    Given default database
    And initialized linter
    And linter ignore
    """
    set_labels_on_cluster:
      - 'warning extra:00000:unused parameter "i_folder_id"'
      - 'warning extra:00000:parameter "$1" is never read'
    # looks like a bug in plpgsql_check
    # probably should create a ticket for it.
    fix_cloud_usage:
      - 'warning:00000:12:SQL statement:too many attributes for composite variable'
      - 'warning:00000:12:SQL statement:too many attributes for target variables'
      - 'Detail: There are less target variables than output columns in query.'
      - 'Hint: Check target variables in SELECT INTO statement'
    """
    Then linter find nothing for functions in schema "code"
