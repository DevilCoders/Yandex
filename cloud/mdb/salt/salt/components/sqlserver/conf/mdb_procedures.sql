USE msdb
GO

CREATE OR ALTER PROCEDURE dbo.mdb_is_replica
WITH EXECUTE AS OWNER
AS
BEGIN
SELECT CAST(count(*) as bit) AS is_replica
                FROM sys.dm_hadr_availability_replica_states
                WHERE is_local = 1 and role in (0,2)

END
GO

GRANT EXECUTE ON dbo.mdb_is_replica TO public
GO

CREATE OR ALTER PROCEDURE [dbo].[mdb_return_is_replica]
WITH EXECUTE AS OWNER, ENCRYPTION
AS
BEGIN
	SET NOCOUNT ON
	DECLARE @is_replica TABLE(is_replica bit)
	
	INSERT @is_replica
	EXEC [dbo].[mdb_is_replica]
	
	RETURN  (SELECT TOP 1 is_replica FROM @is_replica)

END
GO

GRANT EXECUTE ON [dbo].[mdb_return_is_replica] TO public
GO

CREATE OR ALTER PROC dbo.mdb_internal_get_user_roles(@login_name sysname = NULL)

WITH
    EXECUTE AS OWNER,
    ENCRYPTION
AS
BEGIN
    SET NOCOUNT ON
    DECLARE @SQLQuery NVARCHAR(4000) = '
	USE [?]
	SELECT 
		db_id() as DB_ID,
		sp.name as LOGIN,
		dbp2.name as ROLE
    
		FROM sys.database_role_members rm
		JOIN sys.database_principals dbp on rm.member_principal_id = dbp.principal_id
		JOIN sys.database_principals dbp2 on rm.role_principal_id = dbp2.principal_id
		JOIN sys.server_principals sp on sp.sid = dbp.sid
		'+ CASE WHEN @login_name IS NOT NULL THEN 'WHERE sp.name like '''+@login_name+'''
'		ELSE ''
		END


    exec sp_MSforeachdb @SQLQuery

END
GO

CREATE OR ALTER PROCEDURE dbo.mdb_sessions_get
    (@session_id smallint = NULL
                                    ,
    @detail_level SMALLINT = 3
                                    ,
    @order_by int = 1
                                    ,
    @order_by_desc bit = 0
									,
    @show_session_wait_stats bit = 0
                                    ,
    @show_version bit = 0)
WITH
    EXECUTE AS OWNER,
    ENCRYPTION
AS
BEGIN
    SET NOCOUNT ON

    IF @show_version = 1
    SELECT '0.0.0.1'
ELSE
BEGIN

        IF @detail_level not between 1 and 4
    THROW 51000, 'Invalid @detail_level. Valid values are 1, 2 and 3', 16

        DECLARE @login_name sysname = (SELECT ORIGINAL_LOGIN()),
        @cmd NVARCHAR(255) = 'KILL ',
        @SQLQuery NVARCHAR(MAX)

        IF (OBJECT_ID('tempdb..#roles') is not NULL)
    DROP TABLE #roles
        CREATE TABLE #roles
        (
            db_id int,
            login_name sysname,
            role_name sysname
        )
        INSERT #roles
        EXEC msdb.dbo.mdb_internal_get_user_roles @login_name

        SELECT @SQLQuery = '
		DECLARE @session_id smallint = '+ISNULL(cast(@session_id as NVARCHAR(10)),'NULL')+'
		,@login_name sysname = '''+cast(@login_name as NVARCHAR(255))+'''
		SELECT
			db_name(es.database_id) as database_name
			,es.session_id
			,es.login_time
			,es.host_name
			,es.program_name
			,es.login_name
			,es.status
			,es.cpu_time session_cpu_time
			,es.last_request_start_time
			,es.database_id session_database_id
			,es.open_transaction_count
		'+CASE WHEN @detail_level > 1 THEN
		'    ,er.start_time request_start_time
			,er.status request_status
			,er.command request_command
			,er.blocking_session_id
			,er.wait_type
			,er.wait_time
			,er.last_wait_type
			,er.wait_resource
		'
			 ELSE ''
		END
		+
		CASE WHEN @detail_level > 2 THEN 
		'    ,qt.objectid as object_id 
			,qt.text query_text
			,qp.query_plan
		'
		ELSE ''
		END
		+
		CASE WHEN @detail_level > 3 THEN 
		'    ,mg.dop
			 ,mg.scheduler_id
			 ,mg.grant_time
			 ,mg.requested_memory_kb
			 ,mg.granted_memory_kb
			 ,mg.required_memory_kb
			 ,mg.used_memory_kb
			 ,mg.max_used_memory_kb
			 ,mg.query_cost
			 ,mg.timeout_sec
			 ,mg.wait_order
			 ,mg.is_next_candidate
			 ,mg.reserved_worker_count
			 ,mg.used_worker_count
			 ,mg.max_used_worker_count

		'
		ELSE ''
		END

			+'
		FROM #roles d
			JOIN sys.dm_exec_sessions es
			ON  es.database_id = d.db_id and (es.session_id = @session_id or @session_id is null)
			AND es.session_id > 50
			AND es.login_name not in (''sa'',''NT AUTHORITY\SYSTEM'')
			AND d.role_name in (''dbo'',''db_owner'', ''db_securityadmin'')
		'
		+
		CASE WHEN @detail_level > 1 THEN ' LEFT JOIN sys.dm_exec_requests er
				ON es.session_id = er.session_id'
				ELSE ''
				END
		+
		CASE WHEN @detail_level > 2 THEN '
			OUTER APPLY sys.dm_exec_query_plan(er.plan_handle) qp
			OUTER APPLY sys.dm_exec_sql_text(er.sql_handle) qt'
		ELSE ''
		END
		+ 
		CASE WHEN @detail_level > 3 THEN '
			LEFT JOIN sys.dm_exec_query_memory_grants mg
				ON mg.session_id = es.session_id'
		ELSE ''
		END
		+ '
		ORDER BY '+CAST(@order_by as NVARCHAR(10))+' ' + CASE @order_by_desc WHEN 1 THEN 'DESC' ELSE 'ASC' END

		+ '
		OPTION (FORCE ORDER)'
        EXEC(@SQLQuery)

        IF @show_session_wait_stats = 1
			BEGIN
            SELECT
                db_name(es.database_id) database_name,
                ws.*
            FROM #roles d
                JOIN sys.dm_exec_sessions es
                ON  es.database_id = d.db_id and (es.session_id = @session_id or @session_id is null)
                    AND es.session_id > 50
                    AND es.login_name not in ('sa','NT AUTHORITY\SYSTEM')
                    AND d.role_name in ('dbo','db_owner', 'db_securityadmin')
                JOIN sys.dm_exec_session_wait_stats ws on ws.session_id = es.session_id
            ORDER BY es.database_id, es.session_id
        END
    END
END

GO

GRANT EXECUTE ON dbo.mdb_sessions_get TO PUBLIC

GO


CREATE OR ALTER PROCEDURE dbo.mdb_sessions_kill
    (@session_id int = NULL)
WITH
    EXECUTE AS OWNER,
    ENCRYPTION
AS
BEGIN
    DECLARE		@login_name sysname = ORIGINAL_LOGIN(),
				@cmd NVARCHAR(255) = 'KILL '

    IF (OBJECT_ID('tempdb..#roles') is not NULL)
    DROP TABLE #roles
    CREATE TABLE #roles
    (
        db_id int,
        login_name sysname,
        role_name sysname
    )
    INSERT #roles
    EXEC msdb.dbo.mdb_internal_get_user_roles @login_name

    IF @login_name in (
    SELECT d.login_name
    FROM #roles d
        JOIN sys.dm_exec_sessions es
        ON  es.database_id = d.db_id and es.session_id = @session_id
            AND d.role_name in ('dbo','db_owner', 'db_securityadmin')
)
	BEGIN
        SET @cmd = @cmd + ' ' + CAST(@session_id as NVARCHAR(10))
        BEGIN TRY
					EXEC (@cmd)
					PRINT ('Session '+ CAST(@session_id as NVARCHAR(10)) + ' has been killed')
				END TRY
				BEGIN CATCH
					;THROW
				END CATCH
    END
				ELSE  
					THROW 51000, 'Session not found or you are not authorized to kill that kind of sessions!',16
END


GO

GRANT EXECUTE ON dbo.mdb_sessions_kill TO PUBLIC

USE master
GO

IF EXISTS (SELECT 1 FROM sys.server_triggers WHERE name LIKE 'MDB_Prohibit_DROP_DATABASE')
    DROP TRIGGER MDB_Prohibit_DROP_DATABASE ON ALL SERVER
GO
IF EXISTS (SELECT 1 FROM sys.server_triggers WHERE name LIKE 'MDB_Restrict_RecoveryModel_Changes')
    DROP TRIGGER MDB_Restrict_RecoveryModel_Changes ON ALL SERVER
GO

USE msdb
GO

CREATE OR ALTER PROCEDURE dbo.mdb_internal_is_user_rolemember
	@role_name sysname
WITH ENCRYPTION
AS

BEGIN
	IF @role_name not in (
		SELECT name FROM 
			sys.database_principals
			WHERE type = 'R' and is_fixed_role = 1)
		THROW 51000, 'The role is not known as a fixed database role', 16

	DECLARE		@login_name sysname = ORIGINAL_LOGIN(),
				@return bit = 0

    IF (OBJECT_ID('tempdb..#roles') is not NULL)
    	DROP TABLE #roles
    CREATE TABLE #roles
    (
        db_id int,
        login_name sysname,
        role_name sysname
    )
    INSERT #roles
    EXEC msdb.dbo.mdb_internal_get_user_roles @login_name
	IF @login_name in (
		SELECT login_name 
			FROM #roles
			WHERE role_name = @role_name
		)
		SET @return = 1
	ELSE
		SET @return = 0

	RETURN @return
END
GO


CREATE OR ALTER PROCEDURE dbo.mdb_freeproccache 
				@plan_handle varbinary(64) = NULL, 
				@sql_handle varbinary(64) = NULL, 
				@pool_name sysname = NULL,
				@show_version bit = 0

WITH EXECUTE AS OWNER, ENCRYPTION

AS
BEGIN
	SET NOCOUNT ON
	DECLARE @vers NVARCHAR(255)
	SET @vers = '0.0.0.1'
	IF @show_version = 1
		SELECT @vers as [version]

	DECLARE @SQL NVARCHAR(4000)

	IF @pool_name IS NOT NULL AND @pool_name NOT IN (SELECT name FROM sys.dm_resource_governor_resource_pools)
		THROW 510000, 'Pool_name argument is incorrect, no such pool found in the system.', 16
	IF @pool_name IS NOT NULL AND @sql_handle IS NOT NULL
		OR @plan_handle IS NOT NULL AND @sql_handle IS NOT NULL
		OR @plan_handle IS NOT NULL AND @pool_name IS NOT NULL
		THROW 510000, 'Only one of @plan_handle, @sql_handle or @pool_name can be specified at once.', 16

	-----------------MDB role membership validation-------------------
	DECLARE @UserHasAccess bit = 0

	EXEC @UserHasAccess = msdb.dbo.mdb_internal_is_user_rolemember 'db_owner'
	IF @UserHasAccess = 0
	THROW 51000, 'You do not have permission to run database maintenance', 16
	----------------END MDB role membership validation----------------
	

	SET @SQL = 'DBCC FREEPROCCACHE ' + CASE WHEN @sql_handle IS NOT NULL THEN '('+ CAST(@sql_handle as NVARCHAR(255)) + ') ' ELSE '' END
		+ CASE WHEN @plan_handle IS NOT NULL THEN '('+ CAST(@plan_handle as NVARCHAR(255))  + ') ' ELSE '' END
		+ CASE WHEN @pool_name IS NOT NULL THEN '(''' + @pool_name + ''')' ELSE '' END
		+ ' WITH NO_INFOMSGS;'
	EXEC sp_executesql @SQL
	PRINT @SQL

END

GO

GRANT EXECUTE ON msdb.dbo.mdb_freeproccache TO public
GO

CREATE OR ALTER PROCEDURE dbo.mdb_readerrorlog @p1 INT = NULL, @p2 INT = NULL, @p3 NVARCHAR(4000) = NULL, @p4 NVARCHAR(4000) = NULL, @p5 DATETIME = NULL, @p6 DATETIME = NULL, @p7 NVARCHAR(10) = NULL
WITH EXECUTE AS OWNER, ENCRYPTION
AS
BEGIN
	SET NOCOUNT ON

	-----------------MDB role membership validation-------------------
	DECLARE @UserHasAccess BIT = 0

	EXEC @UserHasAccess = msdb.dbo.mdb_internal_is_user_rolemember 'db_owner'
	IF @UserHasAccess = 0
	THROW 51000, 'You do not have permission to access errorlog', 16
	----------------END MDB role membership validation----------------

	EXEC xp_readerrorlog @p1, @p2, @p3, @p4, @p5, @p6, @p7
END
GO

GRANT EXECUTE ON dbo.mdb_readerrorlog to public
GO

IF NOT EXISTS(SELECT 1 FROM sys.server_event_sessions WHERE name = N'replica_state_change')
	BEGIN
	CREATE EVENT SESSION [replica_state_change] ON SERVER 
	ADD EVENT sqlserver.availability_replica_state_change
	ADD TARGET package0.event_file(SET filename=N'replica_state_change',max_file_size=(10))
	WITH (MAX_MEMORY=4096 KB,EVENT_RETENTION_MODE=NO_EVENT_LOSS,MAX_DISPATCH_LATENCY=1 SECONDS,MAX_EVENT_SIZE=0 KB,MEMORY_PARTITION_MODE=NONE,TRACK_CAUSALITY=OFF,STARTUP_STATE=ON)
	ALTER EVENT SESSION [replica_state_change] ON SERVER 
	STATE = START;
	END
