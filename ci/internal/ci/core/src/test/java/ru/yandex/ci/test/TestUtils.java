package ru.yandex.ci.test;

import java.io.IOException;
import java.io.InputStreamReader;
import java.io.UncheckedIOException;
import java.lang.reflect.Field;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Function;
import java.util.function.Predicate;
import java.util.stream.Collectors;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.google.gson.JsonElement;
import com.google.gson.JsonParser;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonToken;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.Message;
import com.yandex.ydb.table.YdbTable;
import com.yandex.ydb.table.values.proto.ProtoType;
import lombok.extern.slf4j.Slf4j;
import org.assertj.core.api.Assertions;

import ru.yandex.ci.common.grpc.ProtobufTestUtils;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.TransactionSupportDefault;
import ru.yandex.ci.common.ydb.YdbExecutor;
import ru.yandex.ci.core.config.a.AYamlParser;
import ru.yandex.ci.core.proto.ProtobufSerialization;
import ru.yandex.ci.util.CiJson;
import ru.yandex.ci.util.FileUtils;
import ru.yandex.ci.util.ResourceUtils;

@Slf4j
public class TestUtils {

    private TestUtils() {
    }

    public static Predicate<Field> fieldsOfClass(Class<?> type, String... fields) {
        Set<String> fieldNames = Set.of(fields);
        return field -> field.getDeclaringClass() == type && fieldNames.contains(field.getName());
    }

    public static String textResource(String resource) {
        return ResourceUtils.textResource(resource);
    }

    public static byte[] binaryResource(String resource) {
        return ResourceUtils.binaryResource(resource);
    }

    public static <T> T parseJson(String resource, Class<T> target) {
        return FileUtils.parseJson(resource, target);
    }

    public static <T> T parseJson(String resource, TypeReference<T> target) {
        return FileUtils.parseJson(resource, target);
    }

    public static ObjectNode readJson(Object object) {
        return (ObjectNode) CiJson.readTree(object);
    }

    public static ObjectNode readJson(String resource) {
        var json = TestUtils.textResource(resource);
        return (ObjectNode) CiJson.readTree(json);
    }

    public static ObjectNode readJsonFromString(String json) {
        return (ObjectNode) CiJson.readTree(json);
    }

    public static JsonElement parseGson(String resource) {
        return JsonParser.parseString(ResourceUtils.textResource(resource));
    }

    public static <T extends Message> T parseProtoText(String resource, Class<T> target) {
        return ProtobufTestUtils.parseProtoText(resource, target);
    }

    public static <T extends Message> T parseProtoTextFromString(String protoText, Class<T> target) {
        return ProtobufTestUtils.parseProtoTextFromString(protoText, target);
    }

    public static <T extends Message> T parseProtoBinary(String resource, Class<T> target) {
        return ProtobufTestUtils.parseProtoBinary(resource, target);
    }

    public static List<JsonElement> readGsonStream(String resource) {
        try (var sourceStream = ResourceUtils.url(resource).openStream()) {
            log.debug("Reading data from {}", resource);
            JsonReader reader = new JsonReader(new InputStreamReader(sourceStream, StandardCharsets.UTF_8));
            List<JsonElement> result = new ArrayList<>();
            int element = 0;
            reader.setLenient(true);
            while (reader.hasNext() && reader.peek() != JsonToken.END_DOCUMENT) {
                log.debug("Reading item #{}", element++);
                var nextElement = JsonParser.parseReader(reader);
                result.add(nextElement);
            }
            return result;
        } catch (IOException e) {
            throw new UncheckedIOException("Failed to read file %s".formatted(resource), e);
        }
    }

    public static <T extends TransactionSupportDefault> UpsertInterface forTable(
            T tx,
            Function<T, KikimrTableCi<?>> tableSource) {
        // We must reset cleanup status for table
        var tableName = tx.currentOrReadOnly(() -> {
            var table = tableSource.apply(tx);
            var rows = table.findAll();
            Assertions.assertThat(rows).isEmpty();
            return table.getTableName();
        });
        return (executor, source) -> upsertValues(executor, tableName, source);
    }

    /**
     * Upset values from data dump into specified ydb table.
     * <p>
     * Prepare source file using following command:
     * <pre>
     * ya ydb -e ydb-ru.yandex.net:2135 -d "/ru/ci/stable/ci" \
     *   table query execute -q \
     *   'select * from  `/ru/ci/stable/ci/flow/FlowLaunch` limit 7' \
     *   --format json-unicode | jq . | pbcopy
     * </pre>
     * <p>
     * If schema isn't dumped or it needs to be updated it can be done in the following way:
     * <pre>
     * ya ydb -e ydb-ru.yandex.net:2135 -d "/ru/ci/stable/ci" \
     *   scheme describe '/ru/ci/stable/ci/flow/FlowLaunch' \
     *   --format proto-json-base64 | jq . | pbcopy
     * </pre>
     *
     * @param executor ydb executor
     * @param table    target table name
     * @param source   path to source file with data in classpath
     */
    private static List<JsonElement> upsertValues(YdbExecutor executor, String table, String source) {

        var schemaFilePath = "ydb-schemas/" + table + ".json";
        String schemaJson;
        try {
            schemaJson = TestUtils.textResource(schemaFilePath);
        } catch (UncheckedIOException e) {
            throw new RuntimeException("File %s not found in resources. Create it if needed".formatted(schemaFilePath));
        }

        Collection<String> tuples;
        Map<String, String> columnTypes;
        try {
            var tableSchema = ProtobufSerialization.deserializeFromJsonString(schemaJson,
                    YdbTable.DescribeTableResult.newBuilder());

            columnTypes = tableSchema.getColumnsList().stream()
                    .collect(Collectors.toMap(
                            YdbTable.ColumnMeta::getName,
                            column -> ProtoType.fromPb(column.getType()).toString()
                    ));
            tuples = columnTypes.entrySet().stream()
                    .map(e -> e.getKey() + ":" + e.getValue())
                    .collect(Collectors.toList());

        } catch (InvalidProtocolBufferException e) {
            throw new UncheckedIOException("Failed parsing %s file to protobuf. Check than file created in a right way"
                    .formatted(schemaFilePath), e);
        }

        List<JsonElement> rows = readGsonStream(source);
        log.info("Inserting {} rows from {} into {}", rows.size(), source, table);

        executor.executeRW(connection -> {
            var ps = connection.prepareStatement("""
                    declare $values as List<Struct<%s>>;
                    UPSERT INTO `%s` select * from as_table($values)
                    """.formatted(String.join(",", tuples), table));

            for (var rowElement : rows) {

                Set<String> emptyColumns = new HashSet<>(columnTypes.keySet());
                for (Map.Entry<String, JsonElement> entry : rowElement.getAsJsonObject().entrySet()) {
                    var column = entry.getKey();
                    var value = entry.getValue();

                    var columnType = columnTypes.get(column);
                    if (columnType == null) {
                        throw new RuntimeException("unknown column '%s'. Update table schema"
                                .formatted(column));
                    }
                    emptyColumns.remove(column);

                    if (value.isJsonNull()) {
                        ps.setNull(column, 0);
                    } else if (value.isJsonPrimitive()) {
                        var primValue = value.getAsJsonPrimitive();
                        if (primValue.isBoolean()) {
                            ps.setBoolean(column, primValue.getAsBoolean());
                        } else if (primValue.isNumber()) {
                            if (columnType.startsWith("Int64")) {
                                ps.setLong(column, primValue.getAsLong());
                            } else if (columnType.startsWith("Int32")) {
                                ps.setInt(column, primValue.getAsInt());
                            } else {
                                ps.setDouble(column, primValue.getAsDouble());
                            }
                        } else if (primValue.isString()) {
                            ps.setString(column, primValue.getAsString());
                        }
                    } else {
                        ps.setString(column, value.getAsString());
                    }
                }

                for (var column : emptyColumns) {
                    ps.setNull(column, 0);
                }

                ps.addBatch();
            }

            ps.executeBatch();
        });
        log.info("Inserted {} rows", rows.size());
        return rows;
    }

    public static <T> T parseYamlPartial(String configYamlPart, TypeReference<T> typeRef) {
        try {
            return AYamlParser.getMapper().readValue(configYamlPart, typeRef);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }

    public interface UpsertInterface {
        List<JsonElement> upsertValues(YdbExecutor executor, String source);
    }
}
