Feature: Check default versions triggers

    Background: Default database with cloud and folder
        Given default database

    Scenario: Insert version in transaction works
        Given new transaction
        When I execute in transaction
        """
        INSERT INTO dbaas.default_versions
            (
                type,
                component,
                major_version,
                minor_version,
                package_version,
                env,
                is_deprecated,
                is_default,
                updatable_to,
                name,
                edition
            ) values (
                'postgresql_cluster',
                'postgres',
                '5',
                '5.1',
                'some_deb_5.1',
                'dev',
                false,
                false,
                '{"6"}',
                '5',
                ''
            );
        """
        Then it success
        When I commit transaction
        Then it fail with error "ERROR:  Incorrect name 6 in updatable_to of default_version (type postgresql_cluster, component postgres, name 5, env dev)"
        When I start transaction
        When I execute in transaction
        """
        INSERT INTO dbaas.default_versions
            (
                type,
                component,
                major_version,
                minor_version,
                package_version,
                env,
                is_deprecated,
                is_default,
                updatable_to,
                name,
                edition
            ) values (
                'postgresql_cluster',
                'postgres',
                '5',
                '5.1',
                'some_deb_5.1',
                'dev',
                false,
                false,
                '{"6"}',
                '5',
                ''
            );
        """
        Then it success
        When I execute in transaction
        """
        INSERT INTO dbaas.default_versions
            (
                type,
                component,
                major_version,
                minor_version,
                package_version,
                env,
                is_deprecated,
                is_default,
                updatable_to,
                name,
                edition
            ) values (
                'postgresql_cluster',
                'postgres',
                '6',
                '6.1',
                'some_deb_6.1',
                'dev',
                false,
                false,
                null,
                '6',
                ''
            );
        """
        Then it success
        When I commit transaction
        Then it success
        When I execute query
        """
        select name, updatable_to from dbaas.default_versions where name = '5' or name = '6';
        """
        Then it returns "2" rows matches
        """
        - name: '5'
          updatable_to:
            - '6'
        - name: '6'
          updatable_to: null
        """


    Scenario: Update name of version in transaction works
        Given new transaction
        When I execute in transaction
        """
        UPDATE dbaas.default_versions SET name = '7' where name = '6';
        """
        Then it success
        When I commit transaction
        Then it fail with error "ERROR:  Old name of default_version (type postgresql_cluster, component postgres, name 6, env dev) contained in updatable_to (type postgresql_cluster, component postgres, name 5, env dev)"
        When I start transaction
        When I execute in transaction
        """
        UPDATE dbaas.default_versions SET name = '7' where name = '6';
        """
        Then it success
        When I execute in transaction
        """
        UPDATE dbaas.default_versions SET updatable_to = '{"7"}' where name = '5';
        """
        Then it success
        When I commit transaction
        Then it success
        When I execute query
        """
        select name, updatable_to from dbaas.default_versions where name = '5' or name = '7';
        """
        Then it returns "2" rows matches
        """
        - name: '5'
          updatable_to:
            - '7'
        - name: '7'
          updatable_to: null
        """
        When I execute query
        """
        select name, updatable_to from dbaas.default_versions where name = '6';
        """
        Then it returns nothing

    Scenario: Update updatable_to of version in transaction works
        Given new transaction
        When I execute in transaction
        """
        UPDATE dbaas.default_versions SET updatable_to = '{"6"}' where name = '5';
        """
        Then it success
        When I commit transaction
        Then it fail with error "ERROR:  Incorrect name 6 in updatable_to of default_version (type postgresql_cluster, component postgres, name 5, env dev)"
        When I start transaction
        When I execute in transaction
        """
        UPDATE dbaas.default_versions SET updatable_to = '{"6"}' where name = '5';
        """
        Then it success
        When I execute in transaction
        """
        UPDATE dbaas.default_versions SET name = '6' where name = '7';
        """
        Then it success
        When I commit transaction
        Then it success
        When I execute query
        """
        select name, updatable_to from dbaas.default_versions where name = '5' or name = '6';
        """
        Then it returns "2" rows matches
        """
        - name: '5'
          updatable_to:
            - '6'
        - name: '6'
          updatable_to: null
        """
        When I execute query
        """
        select name, updatable_to from dbaas.default_versions where name = '7';
        """
        Then it returns nothing

    Scenario: Delete version in transaction works
        Given new transaction
        When I execute in transaction
        """
        DELETE FROM dbaas.default_versions where name = '6';
        """
        Then it success
        When I commit transaction
        Then it fail with error "ERROR:  Old name of default_version (type postgresql_cluster, component postgres, name 6, env dev) contained in updatable_to (type postgresql_cluster, component postgres, name 5, env dev)"
        When I start transaction
        When I execute in transaction
        """
        DELETE FROM dbaas.default_versions where name = '6';
        """
        Then it success
        When I execute in transaction
        """
        DELETE FROM dbaas.default_versions where name = '5';
        """
        Then it success
        When I commit transaction
        Then it success
