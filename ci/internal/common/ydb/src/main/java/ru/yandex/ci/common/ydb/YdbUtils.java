package ru.yandex.ci.common.ydb;

import java.util.Comparator;
import java.util.List;
import java.util.function.Function;
import java.util.function.Predicate;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.net.HostAndPort;
import com.google.common.primitives.UnsignedLong;
import com.google.protobuf.ProtocolMessageEnum;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;

import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.EntitySchema;
import yandex.cloud.repository.kikimr.KikimrConfig;
import yandex.cloud.repository.kikimr.KikimrRepository;
import yandex.cloud.repository.kikimr.auth.KikimrAuthConfig;
import yandex.cloud.repository.kikimr.compatibility.KikimrSchemaCompatibilityChecker;
import ru.yandex.ci.util.SerializationChecks;
import ru.yandex.ci.ydb.Persisted;

@Slf4j
public class YdbUtils {
    public static final int RESULT_ROW_LIMIT = 1000;

    // Maximum number of rows we could fetch using normal `select` operation
    // If you need more - use `scan()` or `readTable()` operations
    public static final int SELECT_LIMIT = 16384;

    private static final boolean YDB_SKIP_CHECKS = Boolean.getBoolean("ydb.skip.checks");

    private YdbUtils() {
    }

    /**
     * Return call location (i.e. backtrace with provided level)
     *
     * @param level to traverse back
     * @return "className:line"
     */
    public static String location(int level) {
        var frame = StackWalker.getInstance().walk(stream -> stream
                .skip(level + 1) // +current
                .findFirst())
                .orElseThrow(() ->
                        new IllegalStateException("Unable to obtain stack trace"));
        var className = frame.getClassName();
        var lastIndexOf = className.lastIndexOf('.');
        return className.substring(lastIndexOf + 1) + ":" + frame.getLineNumber();
    }

    public static KikimrConfig createConfig(
            @Nonnull String endpoint,
            @Nonnull String database,
            @Nullable String token,
            @Nullable String subdir
    ) {
        return createConfig(HostAndPort.fromString(endpoint), database, token, subdir);
    }


    public static KikimrConfig createConfig(
            @Nonnull HostAndPort endpoint,
            @Nonnull String database,
            @Nullable String token,
            @Nullable String subdir
    ) {

        String tablespace = StringUtils.isEmpty(subdir)
                ? database
                : database + "/" + subdir;

        KikimrConfig config = KikimrConfig.create(
                endpoint.getHost(),
                endpoint.getPort(),
                tablespace,
                database
        );
        config = config.withDiscoveryEndpoint(endpoint.toString());

        if (token != null) {
            config = config.withAuth(KikimrAuthConfig.token(token));
        }

        return config;
    }

    @SuppressWarnings({"rawtypes", "unchecked"})
    public static <T extends KikimrRepository> T initDb(
            T repository,
            List<Class<? extends Entity>> schemaEntities
    ) {
        log.info("Running YDB checks for {}", repository);

        if (YDB_SKIP_CHECKS) {
            log.info("Skip YDB checks...");
            return repository; //---
        }

        repository.createTablespace();

        schemaEntities.stream()
                .map((Function<Class<? extends Entity>, EntitySchema>) EntitySchema::of)
                .sorted(Comparator.comparing(EntitySchema::getName))
                .forEach(entity -> {
                    log.info("Checking ORM table: {}/{}",
                            StringUtils.stripEnd(repository.getConfig().getTablespace(), "/"), entity.getName());
                    repository.schema(entity.getType()).create();
                });
        new KikimrSchemaCompatibilityChecker(schemaEntities, repository).run();

        SerializationChecks.checkMarkers(schemaEntities, new Predicate<>() {
            @Override
            public boolean test(Class<?> aClass) {
                if (aClass.getAnnotation(Persisted.class) != null) {
                    return true;
                }
                if (aClass.equals(UnsignedLong.class)) {
                    return true;
                }
                for (var iface : aClass.getInterfaces()) {
                    if (Entity.class.equals(iface) ||
                            Entity.Id.class.equals(iface) ||
                            ProtocolMessageEnum.class.equals(iface)) { // Well, shit
                        return true;
                    }
                }
                return false;
            }

            @Override
            public String toString() {
                return String.format("Must implement %s, %s %s, be %s or marked with annotation %s",
                        Entity.class.getSimpleName(),
                        Entity.Id.class.getSimpleName(),
                        ProtocolMessageEnum.class.getSimpleName(),
                        UnsignedLong.class.getSimpleName(),
                        Persisted.class);
            }
        });


        return repository;
    }
}
