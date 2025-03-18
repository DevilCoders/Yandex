package ru.yandex.ds.pgdriver;

import java.sql.DriverPropertyInfo;
import java.util.Properties;

import javax.annotation.Nonnull;

import org.postgresql.PGProperty;
import org.postgresql.util.GT;
import org.postgresql.util.PSQLException;
import org.postgresql.util.PSQLState;

/**
 * Аналог {@link PGProperty} для параметров относящихся к {@link PGClusterDriver}.
 */
public enum PGClusterProperty {

    MAX_REPLICATION_LAG(
            "maxReplicationLag",
            "10000",
            "Max replication lag in ms. for slave servers which possible for use"
    );

    private final String name;
    private final String defaultValue;
    private final boolean required;
    private final String description;
    private final String[] choices;

    PGClusterProperty(@Nonnull String name, String defaultValue, @Nonnull String description) {
        this(name, defaultValue, description, false);
    }

    PGClusterProperty(@Nonnull String name, String defaultValue, @Nonnull String description, boolean required) {
        this(name, defaultValue, description, required, null);
    }

    PGClusterProperty(@Nonnull String name, String defaultValue, @Nonnull String description, boolean required,
                      String[] choices) {
        this.name = name;
        this.defaultValue = defaultValue;
        this.required = required;
        this.description = description;
        this.choices = choices;
    }

    /**
     * Returns the name of the connection parameter. The name is the key that must be used in JDBC URL
     * or in Driver properties
     *
     * @return the name of the connection parameter
     */
    @Nonnull
    public String getName() {
        return name;
    }

    /**
     * Returns the default value for this connection parameter.
     *
     * @return the default value for this connection parameter or null
     */
    public String getDefaultValue() {
        return defaultValue;
    }

    /**
     * Returns whether this parameter is required.
     *
     * @return whether this parameter is required
     */
    public boolean isRequired() {
        return required;
    }

    /**
     * Returns the description for this connection parameter.
     *
     * @return the description for this connection parameter
     */
    @Nonnull
    public String getDescription() {
        return description;
    }

    /**
     * Returns the available values for this connection parameter.
     *
     * @return the available values for this connection parameter or null
     */
    public String[] getChoices() {
        return choices;
    }

    /**
     * Returns the value of the connection parameters according to the given {@code Properties} or the
     * default value.
     *
     * @param properties properties to take actual value from
     * @return evaluated value for this connection parameter
     */
    public String get(Properties properties) {
        return properties.getProperty(name, defaultValue);
    }

    /**
     * Return the int value for this connection parameter in the given {@code Properties}.
     *
     * @param properties properties to take actual value from
     * @return evaluated value for this connection parameter converted to int
     * @throws PSQLException if it cannot be converted to int.
     */
    @SuppressWarnings("nullness:argument.type.incompatible")
    public int getInt(Properties properties) throws PSQLException {
        String value = get(properties);
        try {
            //noinspection ConstantConditions
            return Integer.parseInt(value);
        } catch (NumberFormatException nfe) {
            throw new PSQLException(GT.tr("{0} parameter value must be an integer but was: {1}",
                    getName(), value), PSQLState.INVALID_PARAMETER_VALUE, nfe);
        }
    }

    /**
     * Convert this connection parameter and the value read from the given {@code Properties} into a
     * {@code DriverPropertyInfo}.
     *
     * @param properties properties to take actual value from
     * @return a DriverPropertyInfo representing this connection parameter
     */
    public DriverPropertyInfo toDriverPropertyInfo(Properties properties) {
        DriverPropertyInfo propertyInfo = new DriverPropertyInfo(name, get(properties));
        propertyInfo.required = required;
        propertyInfo.description = description;
        propertyInfo.choices = choices;
        return propertyInfo;
    }
}
