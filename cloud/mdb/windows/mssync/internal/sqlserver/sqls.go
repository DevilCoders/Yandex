package sqlserver

const QueryGetRoleChangeTimes = `SET NOCOUNT ON
					SET QUOTED_IDENTIFIER ON
					DECLARE @events table (ag_name sysname,current_state sysname, previous_state sysname, logdate datetime)
					;WITH cte_HADR AS (SELECT object_name, CONVERT(XML, event_data) AS data
					FROM sys.fn_xe_file_target_read_file('replica_state*.xel', null, null, null)
					WHERE object_name = 'availability_replica_state_change'
					),
					CTE_events as (
					SELECT data.value('(/event/@timestamp)[1]','datetime') AS [timestamp],
						data.value('(/event/data[@name=''previous_state'']/text)[1]','varchar(max)') as [previous_state],
						data.value('(/event/data[@name=''current_state'']/text)[1]','varchar(max)') as [current_state],
						data.value('(/event/data[@name=''availability_group_name''])[1]','varchar(255)') as [availability_group_name],
						data.value('(/event/data[@name=''availability_replica_name''])[1]','varchar(255)') as [availability_replica_name],
						ROW_NUMBER() OVER (PARTITION BY data.value('(/event/data[@name=''availability_group_name''])[1]','varchar(255)'), 
														data.value('(/event/data[@name=''previous_state'']/text)[1]','varchar(max)'),
														data.value('(/event/data[@name=''current_state'']/text)[1]','varchar(max)')
									ORDER BY data.value('(/event/@timestamp)[1]','datetime') DESC) as RN
					FROM cte_HADR
					)

					INSERT INTO @events (ag_name,current_state, previous_state, logdate)
					SELECT e.availability_group_name, e.current_state, e.previous_state, e.timestamp
					FROM CTE_events e
					WHERE 
					current_state in ('PRIMARY_NORMAL', 'SECONDARY_NORMAL')
					AND RN=1 

					SELECT 
						ISNULL((SELECT TOP(1) logdate
							FROM @events where current_state = 'SECONDARY_NORMAL' ORDER BY logdate DESC),'1901-01-01 00:00:00') as LastDemotionTime,
						ISNULL((SELECT TOP(1) logdate
							FROM @events where current_state = 'PRIMARY_NORMAL' ORDER BY logdate DESC),'1901-01-01 00:00:00') as LastPromotionTime`

const QueryGetAGDatabases = `SELECT d.database_name, db.state_desc
					FROM 
					sys.dm_hadr_availability_replica_states rs
					JOIN sys.availability_groups ag
						ON rs.group_id = ag.group_id
					JOIN  sys.availability_databases_cluster d
						ON d.group_id = ag.group_id
					JOIN sys.databases db
						ON db.name = d.database_name
					WHERE ag.name = @ag_name`

const QueryGetAllAGDatabases = `SELECT ag.name as ag_name, d.database_name, db.state_desc
					FROM 
					sys.dm_hadr_availability_replica_states rs
					JOIN sys.availability_groups ag
						ON rs.group_id = ag.group_id
					JOIN  sys.availability_databases_cluster d
						ON d.group_id = ag.group_id
					JOIN sys.databases db
						ON db.name = d.database_name`

const QueryGetAGRole = `SELECT role_desc 
					FROM sys.availability_groups ag
					JOIN sys.dm_hadr_availability_replica_states rs 
						ON ag.group_id = rs.group_id
					WHERE
						is_local = 1
					AND 
						ag.name = @ag_name`

const QueryGetSQLServerMetadata = `SELECT SERVERPROPERTY('ComputerNamePhysicalNetBIOS') as Hostname
										, ISNULL(SERVERPROPERTY('InstanceName'), 'MSSQLSERVER') as InstanceName
										, SERVERPROPERTY('ProductMajorVersion') as Version
										, SERVERPROPERTY('Edition') as Edition`

const QueryGetAGs = `SELECT name FROM sys.availability_groups`

const QueryGetInstanceHealth = `DECLARE @out TABLE ( create_time datetime,
								component_type nvarchar(255),
								component_name nvarchar(255),
								state int,
								state_desc nvarchar(255),
								data nvarchar(max)
							)

							INSERT @out
							EXEC sp_server_diagnostics

							SELECT component_name, state, data
							FROM @out`

const QueryAGPromote = `ALTER AVAILABILITY GROUP [%s] FAILOVER`

const QueryGetReplicas = `SELECT DISTINCT gs.primary_replica, ars.is_local
							FROM sys.dm_hadr_availability_group_states gs
							JOIN sys.availability_replicas ar
							ON ar.replica_server_name = gs.primary_replica
							JOIN sys.dm_hadr_availability_replica_states ars
							ON ar.replica_id = ars.replica_id 
							ORDER BY gs.primary_replica`
