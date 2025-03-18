package ru.yandex.ci.client.abc;

import javax.annotation.Nullable;

import lombok.Value;

@Value
public class AbcServiceMemberInfo {
    Service service;
    Role role;
    Person person;

    public String getServiceSlug() {
        return service.getSlug();
    }

    public String getRoleCode() {
        return role.getCode();
    }

    @Value
    public static class Person {
        String login;
    }

    @Value
    public static class Service {
        String slug;

        public String getSlug() {
            return slug.toLowerCase();
        }
    }

    @Value
    public static class Name {
        String en;
        String ru;
    }

    @Value
    public static class Scope {
        @Nullable
        Long id;
        String slug; // Scope slug, not ABC slug
        @Nullable
        Name name;

        public static Scope of(String slug) {
            return new Scope(null, slug, null);
        }
    }

    @Value
    public static class Role {
        @Nullable
        Long id;
        String code;
        @Nullable
        Name name;
        Scope scope;

        public static Role of(String code, Scope scope) {
            return new Role(null, code, null, scope);
        }
    }
}
