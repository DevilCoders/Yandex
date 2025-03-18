package ru.yandex.ds.pgdriver;

import java.sql.Connection;
import java.sql.SQLException;

import javax.naming.Reference;
import javax.sql.XAConnection;
import javax.sql.XADataSource;

import org.checkerframework.checker.nullness.qual.Nullable;
import org.postgresql.core.BaseConnection;
import org.postgresql.xa.PGXAConnection;
import org.postgresql.xa.PGXADataSourceFactory;

/**
 * Аналог {@link org.postgresql.xa.PGXADataSource}, который должен использоваться совместно с {@link PGClusterDriver}.
 */
public class PGClusterXADataSource extends PGClusterBaseDataSource implements XADataSource {

    public XAConnection getXAConnection() throws SQLException {
        return getXAConnection(getUser(), getPassword());
    }

    /**
     * Gets a XA-enabled connection to the PostgreSQL database. The database is identified by the
     * DataSource properties serverName, databaseName, and portNumber. The user to connect as is
     * identified by the arguments user and password, which override the DataSource properties by the
     * same name.
     *
     * @return A valid database connection.
     * @throws SQLException Occurs when the database connection cannot be established.
     */
    public XAConnection getXAConnection(@Nullable String user, @Nullable String password) throws SQLException {
        Connection con = getConnection(user, password);
        return new PGXAConnection((BaseConnection) con);
    }

    public String getDescription() {
        return "Cluster XA-enabled DataSource from " + org.postgresql.util.DriverInfo.DRIVER_FULL_NAME;
    }

    /**
     * Generates a reference using the appropriate object factory.
     */
    protected Reference createReference() {
        return new Reference(getClass().getName(), PGXADataSourceFactory.class.getName(), null);
    }
}
