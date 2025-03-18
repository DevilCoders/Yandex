package ru.yandex.ci.core.abc;

import java.util.List;

import javax.annotation.Nonnull;

import lombok.Value;

@Value
public class ServiceSlugWithRoles {
    @Nonnull
    String slug;
    @Nonnull
    List<String> roles;
}
