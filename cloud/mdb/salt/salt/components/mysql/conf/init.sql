SET GLOBAL super_read_only = OFF;

-- drop all old MDB or default users
SET SESSION group_concat_max_len = 100000;
SET @users = NULL;
SELECT GROUP_CONCAT('\'', User, '\'@\'', Host, '\'') INTO @users FROM mysql.user WHERE User in ('root', 'admin', 'repl', 'monitor');
SET @users = CONCAT('DROP USER ', @users);
PREPARE stmt1 FROM @users;
EXECUTE stmt1;
DEALLOCATE PREPARE stmt1;

-- create admin user for salt	
CREATE USER 'admin'@'{{ salt['grains.get']('fqdn') }}' IDENTIFIED BY '{{ salt['pillar.get']('data:mysql:users:admin:password') }}';	
GRANT ALL PRIVILEGES ON *.* TO 'admin'@'{{ salt['grains.get']('fqdn') }}' WITH GRANT OPTION;	
FLUSH PRIVILEGES;
