package ru.yandex.ci.core.db.model;


import javax.annotation.Nonnull;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.Table.View;
import yandex.cloud.repository.kikimr.DbType;

@Value
public class ConfigStateProjectView implements View {
    @Column(dbType = DbType.UTF8)
    @Nonnull
    String project;
}
