package ru.yandex.ci.core.db.autocheck.model;

import javax.annotation.Nonnull;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class AccessControl {
    public static final AccessControl ALL_STAFF = new AccessControl(ACType.ABC, "", "");

    @Nonnull
    @Column(dbType = DbType.STRING)
    ACType type;
    @Column(dbType = DbType.UTF8)
    @Nonnull
    String name;
    @Column(dbType = DbType.UTF8)
    @Nonnull
    String role;

    public static AccessControl ofUser(String userName) {
        return new AccessControl(ACType.USER, userName, "");
    }

    public static AccessControl ofAbc(String serviceName, String role) {
        return new AccessControl(ACType.ABC, serviceName, role);
    }

    @Persisted
    public enum ACType {
        USER,
        ABC
    }
}
