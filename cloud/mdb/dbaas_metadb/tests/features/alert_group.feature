Feature: Alert Group

  Background: Database with cluster
	Given default database
	And cloud with default quota
	And folder
	And cluster

  Scenario: Add alert and delete group with alerts to cluster
	Given "postgres" cluster with
	"""
	name: Postgres
	"""
	When I execute "postgres" cluster change
	"""
	SELECT code.add_alert_group (
		i_ag_id                 => 'ag_id1',
		i_cid                   => :postgres_cid,
		i_monitoring_folder_id  => 'folder1',
		i_managed               => false,
		i_rev                   => :rev
	)
	"""
	Then it success
	When I execute query
	"""
	SELECT alert_group_id as ag_id, status as status
	FROM dbaas.alert_group
	WHERE cid = :postgres_cid;
	"""
	Then it returns one row matches
	"""
	ag_id: ag_id1
	status: CREATING
	"""
	When I execute "postgres" cluster change
	"""
	INSERT INTO dbaas.default_alert VALUES(1, NULL, 'postgresql_cluster', 'postgresql_master_alive',  'test-template-version');
	"""
	Then it success
	When I execute "postgres" cluster change
	"""
	SELECT code.add_alert_to_group (
		i_cid                   => :postgres_cid,
		i_alert_group_id        => 'ag_id1',
		i_template_id           => 'postgresql_master_alive',
		i_crit_threshold        => 1,
		i_warn_threshold        => 1,
		i_notification_channels => '{ch1, ch2}',
		i_disabled              => false,
		i_default_thresholds    => false,
		i_rev                   => :rev
	)
	"""
	Then it success
	When I execute query
	"""
	SELECT template_id as mname, status as status
	FROM dbaas.alert
	WHERE alert_group_id = 'ag_id1';
	"""
	Then it returns one row matches
	"""
	mname: 'postgresql_master_alive'
	status: CREATING
	"""
	When I execute "postgres" cluster change
	"""
	SELECT code.add_alert_to_group (
		i_cid                   => :postgres_cid,
		i_alert_group_id        => 'ag_id1',
		i_template_id           => 'postgresql_foo_bar',
		i_crit_threshold        => 1,
		i_warn_threshold        => 1,
		i_notification_channels => '{ch1, ch2}',
		i_disabled              => false,
		i_default_thresholds    => false,
		i_rev                   => :rev
	)
	"""
    Then it fail with error matches "insert or update on table \"alert\" violates foreign key constraint \"fk_alert_default_alert\""
	When I execute "postgres" cluster change
	"""
	SELECT code.delete_alert_group (
		i_cid                   => :postgres_cid,
		i_alert_group_id        => 'ag_id1',
		i_rev                   => :rev
	)
	"""
	Then it success
	When I execute "postgres" cluster change
	"""
	SELECT code.delete_alert_from_group (
		i_cid                   => :postgres_cid,
		i_alert_group_id        => 'ag_id1',
		i_template_id           => 'postgresql_master_alive',
		i_rev                   => :rev
	)
	"""
	Then it success
  @managed_ag_deletion
  Scenario: Deletion managed alert group fails
	Given "postgres" cluster with
	"""
	name: Postgres
	"""
	When I execute "postgres" cluster change
	"""
	SELECT code.add_alert_group (
		i_ag_id                 => 'ag_id2',
		i_cid                   => :postgres_cid,
		i_monitoring_folder_id  => 'folder1',
		i_managed               => true,
		i_rev                   => :rev
	)
	"""
	Then it success
	When I execute query
	"""
	SELECT code.delete_alert_group (
		i_cid                   => :postgres_cid,
		i_alert_group_id        => 'ag_id2',
		i_rev                   => :rev
	)
	"""
	Then it fail with error matches "Deletion of managed alert group ag_id2 is prohibited"
	When I execute query
	"""
	SELECT code.delete_alert_group (
		i_cid                          => :postgres_cid,
		i_alert_group_id               => 'ag_id2',
		i_rev                          => :rev,
		i_force_managed_group_deletion => TRUE
	)
	"""
	
  @update_alert
  Scenario: Update alert works
	Given "postgres" cluster with
	"""
	name: Postgres
	"""
	When I execute "postgres" cluster change
	"""
	SELECT code.add_alert_group (
		i_ag_id                 => 'ag_id3',
		i_cid                   => :postgres_cid,
		i_monitoring_folder_id  => 'folder1',
		i_managed               => true,
		i_rev                   => :rev
	)
	"""
	When I execute "postgres" cluster change
	"""
	SELECT code.add_alert_to_group (
		i_cid                   => :postgres_cid,
		i_alert_group_id        => 'ag_id3',
		i_template_id           => 'postgresql_master_alive',
		i_crit_threshold        => 1,
		i_warn_threshold        => 1,
		i_notification_channels => '{ch1, ch2}',
		i_disabled              => false,
		i_default_thresholds    => false,
		i_rev                   => :rev
	)
	"""
	Then it success
	When I execute "postgres" cluster change
	"""
	SELECT code.update_alert (
		i_cid                   => :postgres_cid,
		i_alert_group_id        => 'ag_id3',
		i_template_id           => 'postgresql_master_alive',
		i_crit_threshold        => 1,
		i_warn_threshold        => 1,
		i_notification_channels => '{ch1, ch2}',
		i_disabled              => false,
		i_default_thresholds    => false,
		i_rev                   => :rev
	)
	"""
	Then it success
	When I execute query
	"""
	SELECT template_id as mname, status as status
	FROM dbaas.alert
	WHERE alert_group_id = 'ag_id3';
	"""
	Then it returns one row matches
	"""
	mname: 'postgresql_master_alive'
	status: UPDATING
	"""
