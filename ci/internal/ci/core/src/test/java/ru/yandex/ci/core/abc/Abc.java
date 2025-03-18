package ru.yandex.ci.core.abc;

import java.util.List;

import lombok.Value;
import one.util.streamex.StreamEx;

import ru.yandex.ci.client.abc.AbcServiceInfo;

@Value(staticConstructor = "of")
public class Abc {

    public static final Abc SEARCH = Abc.of(7, "meta_search", "Search portal");
    public static final Abc INFRA = Abc.of(8, "meta_infra", "Infra", SEARCH);
    public static final Abc DEVTECH = Abc.of(9, "devtech", "Devtech", SEARCH, INFRA);
    public static final Abc TE = Abc.of(21, "testenv", "Testenv", SEARCH, INFRA, DEVTECH);
    public static final Abc CI = Abc.of(42, "ci", "CI checks", SEARCH, INFRA, DEVTECH);
    public static final Abc AUTOCHECK = Abc.of(43, "autocheck", "Автосборка");
    public static final Abc SERP_SEARCH = Abc.of(1021, "serpsearch", "Выдача поиска (SERP)", List.of(Abc.SEARCH));

    int id;
    String slug;
    String name;
    List<Abc> hierarchy;

    public String getPath() {
        return "/" + StreamEx.of(hierarchy).map(Abc::getSlug).append(slug).joining("/") + "/";
    }

    public AbcServiceInfo toServiceInfo() {
        return new AbcServiceInfo(
                getId(),
                getSlug(),
                new AbcServiceInfo.LocalizedName("Название", getName()),
                new AbcServiceInfo.LocalizedName("Описание", "Description"),
                getPath()
        );
    }

    public static Abc of(int id, String slug, String name, Abc... hierarchy) {
        return of(id, slug, name, List.of(hierarchy));
    }
}
