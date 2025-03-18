package ru.yandex.ci.core.launch;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;

import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.github.fge.jsonschema.core.exceptions.InvalidSchemaException;
import com.github.fge.jsonschema.core.exceptions.ProcessingException;
import com.github.fge.jsonschema.exceptions.InvalidInstanceException;
import com.github.fge.jsonschema.main.JsonSchemaFactory;
import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonSyntaxException;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.StreamEx;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.config.SchemaReports;
import ru.yandex.ci.core.config.a.AYamlParser;
import ru.yandex.ci.core.config.a.model.FlowVarsUi;
import ru.yandex.ci.core.tasklet.SchemaService;
import ru.yandex.ci.util.gson.CiGson;

@Slf4j
@RequiredArgsConstructor
public class FlowVarsService {
    private static final ObjectMapper MAPPER = AYamlParser.getMapper();

    private final SchemaService schemaService;

    @Nullable
    public JsonObject prepareFlowVarsFromUi(@Nullable JsonObject passedFlowVars,
                                            @Nullable JsonObject declaredFlowVars,
                                            @Nullable FlowVarsUi flowVarsUi) {
        return prepareFlowVarsFromUi(passedFlowVars, declaredFlowVars, flowVarsUi, true);
    }

    @Nullable
    public JsonObject prepareFlowVarsFromUi(@Nullable JsonObject passedFlowVars,
                                            @Nullable JsonObject declaredFlowVars,
                                            @Nullable FlowVarsUi flowVarsUi,
                                            boolean failIfInvalid) {
        var result = merge(flowVarsUi, passedFlowVars);
        var validationResult = validate(result, flowVarsUi);
        if (!validationResult.isValid()) {
            var errorMessage = "flow vars (%s), passed from ui: (%s) are not valid according to schema (%s): \n%s"
                    .formatted(
                            result,
                            passedFlowVars,
                            Objects.requireNonNull(flowVarsUi).getSchema(),
                            String.join(",\n", validationResult.getErrors())
                    );
            if (failIfInvalid) {
                throw new FlowVarsValidationException(errorMessage);
            } else {
                log.info(errorMessage);
            }
        }
        // тут на самом деле переопределений не ожидаем
        // в валидном конфиге множества ключей в flowVars и flowVarsUi не пересекаются
        return schemaService.override(declaredFlowVars, result);
    }

    private JsonObject merge(@Nullable FlowVarsUi flowVarsUi, @Nullable JsonObject flowVarsFromUi) {
        var defaultsFromFlowVarsUi = makeDefaults(flowVarsUi);
        return schemaService.override(defaultsFromFlowVarsUi, flowVarsFromUi);
    }


    public ValidationResult validate(@Nullable JsonObject flowVars, @Nullable FlowVarsUi flowVarsUi) {
        if (flowVarsUi == null) {
            return ValidationResult.valid();
        }

        // отсутствие flow-vars считаем пустым объектом, чтобы провалидировать против схемы
        // в схеме поддерживаем только объекты, типа нельзя написать flow-vars: "my-value"
        flowVars = Objects.requireNonNullElse(flowVars, new JsonObject());
        return validateWithJsonSchema(flowVars.toString(), flowVarsUi.getSchema());
    }

    private ValidationResult validateWithJsonSchema(String flowVars, JsonNode schema) {

        // валидатор у нас поддерживает jackson, однако flow vars объекты задаются gson-ом
        // https://st.yandex-team.ru/CI-1584
        JsonNode parsedFlowVars = parseJackson(flowVars, "flow vars");

        try {
            var report = JsonSchemaFactory.byDefault()
                    .getValidator()
                    .validate(schema, parsedFlowVars);
            if (!report.isSuccess()) {
                var errors = SchemaReports.getErrorMessages(report);
                return ValidationResult.invalid(errors);
            }
        } catch (InvalidSchemaException e) {
            throw new FlowVarsValidationException("invalid json schema for flow vars: " + schema, e);
        } catch (InvalidInstanceException e) {
            throw new FlowVarsValidationException("flow vars (%s) is invalid json"
                    .formatted(flowVars), e);
        } catch (ProcessingException e) {
            throw new FlowVarsValidationException("error when validating flow vars (%s) with schema (%s)"
                    .formatted(flowVars, schema), e);
        }

        return ValidationResult.valid();
    }

    private JsonNode parseJackson(String json, String description) {
        try {
            return MAPPER.readTree(json);
        } catch (JsonProcessingException e) {
            throw new FlowVarsValidationException("error when parsing " + description + ": " + json, e);
        }
    }

    @Nullable
    public JsonObject parse(@Nullable Common.FlowVars flowVars) {
        if (flowVars == null || flowVars.getJson().isEmpty()) {
            return null;
        }
        try {
            var element = JsonParser.parseString(flowVars.getJson());
            if (!element.isJsonObject()) {
                throw new FlowVarsValidationException("json is not an object, only objects supported: "
                        + flowVars.getJson());
            }
            return element.getAsJsonObject();
        } catch (JsonSyntaxException e) {
            throw new FlowVarsValidationException("error when parsing flow vars provided from ui: " + flowVars, e);
        }
    }

    private static List<FieldDesc> getFieldsAndDefaults(JsonNode schema) {
        var properties = schema.get("properties");
        if (properties == null) {
            return List.of();
        }
        var required = getRequiredFields(schema);

        return StreamEx.of(properties.fields())
                .map(p -> {
                    var defaultValue = p.getValue().get("default");
                    var field = p.getKey();
                    return new FieldDesc(field, defaultValue, required.contains(field), p.getValue());
                })
                .toList();
    }

    private static Set<String> getRequiredFields(JsonNode schema) {
        return Optional.ofNullable(schema.get("required"))
                .map(array -> {
                    List<String> items = new ArrayList<>();
                    for (var jsonElement : array) {
                        items.add(jsonElement.asText());
                    }
                    return Set.copyOf(items);
                })
                .orElse(Set.of());
    }

    public static List<String> getFieldsWithoutDefaults(JsonNode schema) {
        var required = schema.get("required");
        if (required == null) {
            return List.of();
        }
        var requiredProperties = StreamEx.of(required.elements())
                .map(JsonNode::asText)
                .toList();

        return getFieldsAndDefaults(schema)
                .stream()
                .filter(f -> f.getDefaultJson() == null)
                .map(FieldDesc::getField)
                .filter(requiredProperties::contains)
                .toList();
    }

    @Nullable
    @VisibleForTesting
    JsonObject makeDefaults(@Nullable FlowVarsUi flowVarsUi) {
        if (flowVarsUi == null) {
            return null;
        }

        var result = new JsonObject();
        for (var fieldDesc : getFieldsAndDefaults(flowVarsUi.getSchema())) {
            if (fieldDesc.getDefaultJson() != null) {
                result.add(fieldDesc.getField(), fieldDesc.getDefaultJsonInGson());
            }
        }
        return result;
    }

    @Value
    @AllArgsConstructor(access = AccessLevel.PRIVATE)
    public static class ValidationResult {
        List<String> errors;

        public static ValidationResult valid() {
            return new ValidationResult(List.of());
        }

        public static ValidationResult invalid(List<String> errors) {
            Preconditions.checkNotNull(errors);
            Preconditions.checkArgument(!errors.isEmpty());
            return new ValidationResult(errors);
        }

        public boolean isValid() {
            return errors.isEmpty();
        }
    }

    @Value
    private static class FieldDesc {
        String field;
        JsonNode defaultJson; // может быть как примитивом, так и объектом
        boolean required;
        JsonNode schema; // field schema

        // remove after https://st.yandex-team.ru/CI-1584
        public JsonElement getDefaultJsonInGson() {
            return CiGson.instance().fromJson(defaultJson.toString(), JsonElement.class);
        }
    }
}
