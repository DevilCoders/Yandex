package ru.yandex.ci.util;

import java.lang.reflect.Modifier;
import java.lang.reflect.ParameterizedType;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.function.Predicate;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

@Slf4j
public class SerializationChecks {

    private SerializationChecks() {
        //
    }

    /**
     * Check given class (and all subclasses) is marked with special annotation/implements specific interfaces and so
     * on. This helps tracking classes we must review thoroughly.
     *
     * @param types     classes to check
     * @param typeCheck checker (don't forget to implement {@link Predicate#toString()})
     */
    public static void checkMarkers(Collection<? extends Class<?>> types, Predicate<Class<?>> typeCheck) {
        log.info("Checking classes, matching [{}]", typeCheck);
        var validator = new AnnotationMarkersValidator(typeCheck);
        types.forEach(validator::validate);
        validator.reportErrors();
    }

    @RequiredArgsConstructor
    private static class AnnotationMarkersValidator {
        private static final String[] EXCLUDE_PACKAGE_PREFIXES = new String[]{
                "java.",
                "yandex.cloud.binding",
                "com.google.gson",
                "com.fasterxml.jackson.databind",
                "org.springframework.util.unit", // Well, shit
                "ru.yandex.commune.bazinga."
        };

        @Nonnull
        private final Predicate<Class<?>> predicate;
        private final Set<Class<?>> checkedClasses = new HashSet<>();

        private final List<Class<?>> invalidClasses = new ArrayList<>();

        void validate(Class<?> type) {
            this.resolve(type, 0);
        }

        void reportErrors() {
            invalidClasses.forEach(type -> log.error("Class {} is invalid", type));

            if (invalidClasses.size() > 0) {
                throw new IllegalStateException("Found " + invalidClasses.size() +
                        " invalid class(es): " + invalidClasses + ", [" + predicate + "]");
            }
        }

        void resolve(Class<?> type, int level) {
            if (type.isPrimitive() || type.isInterface()) {
                return; // ---
            }
            if (!checkedClasses.add(type)) {
                return; // ---
            }
            var packageName = type.getPackageName();
            for (var exclude : EXCLUDE_PACKAGE_PREFIXES) {
                if (packageName.startsWith(exclude)) {
                    return; // ---
                }
            }

            if (type.isArray()) {
                resolve(type.getComponentType(), level);
                return; // ---
            }

            log.info("{}Checking class {}...", "+".repeat(level), type);
            if (!predicate.test(type)) {
                invalidClasses.add(type);
            }

            if (type.isEnum()) {
                return; // --- Do not check enum values (all enums must be serialized by name)
            }

            for (var field : type.getDeclaredFields()) {
                var modifiers = field.getModifiers();
                if (Modifier.isStatic(modifiers)) {
                    continue; // ---
                }
                if (Modifier.isTransient(modifiers)) {
                    continue; // ---
                }

                var fieldType = field.getType();
                if (field.getGenericType() instanceof ParameterizedType) {
                    resolveParameterizedType((ParameterizedType) field.getGenericType(), level);
                }
                resolve(fieldType, level + 1);
            }
        }

        void resolveParameterizedType(ParameterizedType parameterizedType, int level) {
            var actualTypes = parameterizedType.getActualTypeArguments();
            for (var actualType : actualTypes) {
                if (actualType instanceof ParameterizedType) {
                    resolveParameterizedType((ParameterizedType) actualType, level + 1);
                } else if (actualType instanceof Class<?>) {
                    resolve((Class<?>) actualType, level + 1);
                } else {
                    throw new IllegalStateException("Unsupported actual type: " + actualType);
                }
            }
        }
    }
}
