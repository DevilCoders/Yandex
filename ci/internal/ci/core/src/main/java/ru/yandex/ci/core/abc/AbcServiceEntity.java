package ru.yandex.ci.core.abc;

import java.time.Instant;
import java.util.List;
import java.util.function.Supplier;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.google.common.base.Splitter;
import com.google.common.base.Suppliers;
import lombok.Builder;
import lombok.EqualsAndHashCode;
import lombok.ToString;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import ru.yandex.ci.client.abc.AbcServiceInfo;
import ru.yandex.ci.client.abc.AbcServiceInfo.LocalizedName;

@Value
@Builder(toBuilder = true)
@Table(name = "main/AbcServices")
@GlobalIndex(name = AbcServiceEntity.IDX_BY_SLUG, fields = {"slug", "name.en"})
public class AbcServiceEntity implements Entity<AbcServiceEntity> {

    public static final String IDX_BY_SLUG = "IDX_BY_SLUG";

    private static final Splitter PATH_SPLITTER = Splitter.on('/').omitEmptyStrings().trimResults();

    @Nonnull
    Id id;

    @Nonnull
    @Column
    String slug;

    @Nonnull
    @Column
    LocalizedName name;

    @Nonnull
    @Column
    LocalizedName nameLower;

    @Nonnull
    @Column
    LocalizedName description;

    @Nonnull
    @Column
    String path;

    @Nonnull
    Instant updated;

    boolean deleted;

    @JsonIgnore
    @ToString.Exclude
    @EqualsAndHashCode.Exclude
    transient Supplier<List<String>> slugHierarchyCache = Suppliers.memoize(() ->
            PATH_SPLITTER.splitToList(getPath()).stream()
                    .map(String::toLowerCase)
                    .toList());

    @Override
    public Id getId() {
        return id;
    }

    public List<String> getSlugHierarchy() {
        return slugHierarchyCache.get();
    }

    public String getUrl() {
        return "https://abc.yandex-team.ru/services/" + slug;
    }

    public static AbcServiceEntity empty(String slug) {
        var name = new LocalizedName("Неизвестный сервис '" + slug + "'", "Unknown abc service '" + slug + "'");
        return AbcServiceEntity.builder()
                .id(Id.of(-1))
                .slug(slug)
                .name(name)
                .nameLower(name.toLower())
                .description(name)
                .path("")
                .updated(Instant.now())
                .build();
    }

    public static AbcServiceEntity root() {
        var name = new LocalizedName("Корневой сервис", "Root");
        return AbcServiceEntity.builder()
                .id(Id.of(-1))
                .slug("")
                .name(name)
                .nameLower(name.toLower())
                .description(name)
                .path("")
                .updated(Instant.now())
                .build();
    }

    public static AbcServiceEntity of(AbcServiceInfo info, Instant updated) {
        return AbcServiceEntity.builder()
                .id(Id.of(info.getId()))
                .slug(info.getSlug())
                .name(info.getName())
                .nameLower(info.getName().toLower())
                .description(info.getDescription())
                .path(info.getPath())
                .updated(updated)
                .build();
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<AbcServiceEntity> {
        @Column(name = "id")
        int id;
    }
}
