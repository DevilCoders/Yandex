package ru.yandex.ci.core.tasklet;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.BiConsumer;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.CaseFormat;
import com.google.common.base.Converter;
import com.google.common.base.Preconditions;
import com.google.common.collect.Sets;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.protobuf.ByteString;
import com.google.protobuf.DescriptorProtos.FileDescriptorSet;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.DynamicMessage;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.Message;
import com.google.protobuf.util.JsonFormat;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.StreamEx;

import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.job.TaskUnrecoverableException;
import ru.yandex.ci.core.proto.ProtobufSerialization;
import ru.yandex.ci.core.resolver.PropertiesSubstitutor;

@Slf4j
public class SchemaService {
    private static final String WRAPPED_OBJECT_KEY = "@@object";

    private static final Converter<String, String> CAMEL_TO_SNAKE_CONVERTER =
            CaseFormat.LOWER_CAMEL.converterTo(CaseFormat.LOWER_UNDERSCORE);

    private static final Converter<String, String> SNAKE_TO_CAMEL_CONVERTER =
            CaseFormat.LOWER_UNDERSCORE.converterTo(CaseFormat.LOWER_CAMEL);

    public void validate(String input, String output, byte[] descriptors) {
        validate(input, output, DescriptorsParser.parseDescriptorSet(descriptors));
    }

    public void validate(String input, String output, FileDescriptorSet descriptorSet) {
        var map = DescriptorsParser.parseDescriptors(descriptorSet);
        if (!map.containsKey(input)) {
            throw new SchemaValidationException("cannot find input message type " + input + " in descriptors");
        }
        if (!map.containsKey(output)) {
            throw new SchemaValidationException("cannot find output message type " + output + " in descriptors");
        }
    }

    @Nullable
    public JsonObject override(@Nullable JsonObject base, @Nullable JsonObject override) {
        return override(base, override, false);
    }

    @Nullable
    public JsonObject override(@Nullable JsonObject base, @Nullable JsonObject override, boolean matchSnakeCase) {
        if (base == null) {
            return override;
        }
        if (override == null) {
            return base;
        }

        JsonObject result = base.deepCopy();
        for (var entry : override.entrySet()) {
            var overrideValue = entry.getValue();
            BiConsumer<String, JsonElement> addValue = (key, resultValue) -> {
                if (resultValue != null && resultValue.isJsonObject() &&
                        overrideValue != null && overrideValue.isJsonObject()) {
                    doOverrideDeep(resultValue.getAsJsonObject(), overrideValue.getAsJsonObject());
                    result.add(key, resultValue);
                } else {
                    result.add(key, overrideValue);
                }
            };

            var key = entry.getKey();
            var resultValue = result.get(key);
            if (!matchSnakeCase) { // No additional checks required
                addValue.accept(key, resultValue);
                continue;
            }

            if (resultValue != null) {
                addValue.accept(key, resultValue);
                continue;
            }

            key = CAMEL_TO_SNAKE_CONVERTER.convert(entry.getKey());
            resultValue = key == null ? null : result.get(key);
            if (resultValue != null) {
                addValue.accept(key, resultValue);
                continue;
            }

            key = SNAKE_TO_CAMEL_CONVERTER.convert(entry.getKey());
            resultValue = key == null ? null : result.get(key);
            if (resultValue != null) {
                addValue.accept(key, resultValue);
                continue;
            }

            // No previous value
            addValue.accept(entry.getKey(), null);
        }
        return result;
    }

    public JsonObject transformOutputToJson(TaskletDescriptor descriptor, ByteString output) {
        var outputMetadata = getMessageDescriptor(descriptor, MessageType.OUTPUT);
        Message message;
        try {
            message = DynamicMessage.newBuilder(outputMetadata).mergeFrom(output).build();
        } catch (InvalidProtocolBufferException e) {
            throw new RuntimeException("Unable to read output message from Proto " + outputMetadata.getFullName(), e);
        }
        return ProtobufSerialization.serializeToGson(message);
    }

    public Message transformInputToProto(TaskletDescriptor descriptor, JsonObject object) {
        var inputMetadata = getMessageDescriptor(descriptor, MessageType.INPUT);
        var builder = DynamicMessage.newBuilder(inputMetadata);
        try {
            JsonFormat.parser().merge(object.toString(), builder);
        } catch (InvalidProtocolBufferException e) {
            throw new RuntimeException("Unable to transform input message to Proto " + inputMetadata.getFullName(), e);
        }
        return builder.build();
    }

    public JsonObject composeInput(TaskletDescriptor descriptor, SchemaOptions options, List<JobResource> inputs) {
        Descriptor inputMetadata = getMessageDescriptor(descriptor, MessageType.INPUT);
        boolean isSingle = options.isSingleInput();

        var inputsDataByParentField = inputs.stream()
                .filter(inp -> inp.getParentField() != null)
                .collect(Collectors.toMap(JobResource::getParentField, JobResource::getData));

        inputs = inputs.stream()
                .filter(inp -> inp.getParentField() == null)
                .collect(Collectors.toList()); // Must be ArrayList

        if (isSingle) {
            JobResourceType inputType = descriptor.getInputType();
            JobResource inputRootResource = popResourceByType(inputType, inputs);

            assertOnlyOptionalResourcesLeft(inputs);

            return inputRootResource.getData();
        }

        var inputResult = new JsonObject();
        for (FieldDescriptor field : inputMetadata.getFields()) {
            var type = getResourceTypeFromField(field, MessageType.INPUT);
            String propertyName = field.getName();
            String propertyJsonName = field.getJsonName();

            if (inputsDataByParentField.containsKey(propertyName)) {
                inputResult.add(propertyName, inputsDataByParentField.remove(propertyName));
            } else if (inputsDataByParentField.containsKey(propertyJsonName)) {
                inputResult.add(propertyName, inputsDataByParentField.remove(propertyJsonName));
            } else if (field.isRepeated()) {
                JsonArray injected = new JsonArray();
                drainResourcesByType(type, inputs).stream()
                        .map(JobResource::getData)
                        .forEach(injected::add);
                inputResult.add(propertyName, injected);
            } else {
                JobResource resource = popResourceByType(type, inputs);
                inputResult.add(propertyName, resource.getData());
            }
        }
        assertOnlyOptionalResourcesLeft(inputs);
        assertNoFieldsInInputRootDataLeft(inputsDataByParentField);

        return inputResult;
    }

    public List<JobResource> resourceDataToResources(TaskletDescriptor taskletDescriptor, JsonObject inputData) {
        Descriptor inputMetadata = getMessageDescriptor(taskletDescriptor, MessageType.INPUT);
        List<JobResource> inputResources = new ArrayList<>();

        validateInput(inputMetadata, inputData);

        for (FieldDescriptor field : inputMetadata.getFields()) {
            var type = getResourceTypeFromField(field, MessageType.INPUT);
            inputResources.addAll(convertResource(
                    type,
                    null,
                    field.isRepeated(),
                    field.getJsonName(),
                    field.getName(),
                    taskletDescriptor.getId(),
                    inputData
            ));
        }

        return inputResources;
    }

    public List<JobResource> extractOutput(TaskletDescriptor descriptor, SchemaOptions options, JsonObject output) {
        Descriptor outputMessage = getMessageDescriptor(descriptor, MessageType.OUTPUT);
        boolean isSingle = options.isSingleOutput();
        if (isSingle) {
            return List.of(JobResource.mandatory(descriptor.getOutputType(), output));
        }

        List<JobResource> resources = new ArrayList<>(outputMessage.getFields().size());
        List<String> errorMessages = new ArrayList<>();
        for (FieldDescriptor field : outputMessage.getFields()) {
            JobResourceType fieldType = getResourceTypeFromField(field, MessageType.OUTPUT);
            String propertyName = field.getName();
            JsonElement jsonElement = output.get(propertyName);

            if (field.isRepeated()) {
                if (jsonElement == null) {
                    // 0 элементов может сериализоваться в json как отсутствие свойства
                    continue;
                }
                if (!jsonElement.isJsonArray()) {
                    errorMessages.add("field " + propertyName + " is declared as repeated in proto," +
                            " array expected in json, but got " + jsonElement.getClass().getSimpleName());
                    continue;
                }

                for (JsonElement arrayElement : jsonElement.getAsJsonArray()) {
                    if (!arrayElement.isJsonObject()) {
                        throw new SchemaException(String.format(
                                "tasklet error (%s): " +
                                        "only repeated messages are supported," +
                                        " expected %s to be an array of objects," +
                                        " but found element %s",
                                descriptor.getId(), propertyName, arrayElement.getClass().getSimpleName()
                        ));
                    }
                    resources.add(JobResource.mandatory(fieldType, arrayElement.getAsJsonObject()));
                }
            } else {
                if (jsonElement == null) {
                    errorMessages.add(
                            "tasklet didn't produce resource " + fieldType.getMessageName() +
                                    " on field '" + propertyName + "'"
                    );
                    continue;
                }
                if (!jsonElement.isJsonObject()) {
                    errorMessages.add(
                            " expected " + propertyName + " to be an object," +
                                    " but found " + jsonElement.getClass().getSimpleName()
                    );
                    continue;
                }

                resources.add(JobResource.mandatory(fieldType, jsonElement.getAsJsonObject()));
            }
        }

        if (!errorMessages.isEmpty()) {
            throw new SchemaException(String.join("; ", errorMessages));
        }

        return resources;
    }

    public TaskletSchema extractSchema(TaskletDescriptor descriptor, SchemaOptions schemaOptions) {
        List<TaskletSchema.Field> input = extractFields(
                descriptor,
                MessageType.INPUT,
                schemaOptions.isSingleInput()
        );
        List<TaskletSchema.Field> output = extractFields(
                descriptor,
                MessageType.OUTPUT,
                schemaOptions.isSingleOutput()
        );
        return new TaskletSchema(input, output);
    }

    public List<JobResource> convertResource(
            JobResourceType type,
            @Nullable Descriptor descriptor,
            boolean repeated,
            String snakeCaseProperty,
            String camelCaseProperty,
            Object objectId,
            JsonObject inputData
    ) {

        var propertyName = inputData.has(snakeCaseProperty)
                ? snakeCaseProperty
                :
                inputData.has(camelCaseProperty)
                        ? camelCaseProperty
                        : null;
        if (propertyName == null) {
            return List.of();
        }

        Function<JsonElement, JsonObject> wrapObject = element -> {
            if (element.isJsonObject()) {
                return element.getAsJsonObject();
            } else if (PropertiesSubstitutor.hasStringSubstitution(element)) {
                return wrapObject(element);
            } else {
                throw new TaskletMetadataValidationException(String.format(
                        "Field %s must be configured with %s or JMESPath got %s, expected for tasklet (%s)",
                        propertyName,
                        repeated ? "array" : "object",
                        element.getClass(),
                        objectId
                ));
            }
        };

        var propertyValue = inputData.get(propertyName);
        if (repeated && propertyValue.isJsonArray()) {
            return StreamEx.of(propertyValue.getAsJsonArray().iterator())
                    .map(wrapObject)
                    .peek(wrappedElement -> {
                        if (descriptor != null) {
                            validateInput(descriptor, wrappedElement);
                        }
                    })
                    .map(wrappedElement -> JobResource.mandatory(type, wrappedElement))
                    .toList();
        } else {
            var wrappedElement = wrapObject.apply(propertyValue);
            if (descriptor != null) {
                validateInput(descriptor, wrappedElement);
            }
            return List.of(JobResource.withParentField(
                    type,
                    wrappedElement,
                    propertyName
            ));
        }
    }

    public static JsonObject wrapObject(JsonElement element) {
        var object = new JsonObject();
        object.add(WRAPPED_OBJECT_KEY, element);
        return object;
    }

    public static JsonElement unwrapObject(JsonObject object) {
        if (object.has(WRAPPED_OBJECT_KEY)) {
            return object.get(WRAPPED_OBJECT_KEY);
        } else {
            return object;
        }
    }

    private static void doOverrideDeep(JsonObject result, JsonObject override) {
        Set<String> commonKeys = Set.copyOf(Sets.intersection(result.keySet(), override.keySet()));
        Set<String> newKeys = Set.copyOf(Sets.difference(override.keySet(), result.keySet()));

        for (String commonKey : commonKeys) {
            JsonElement overrideValue = override.get(commonKey);
            if (overrideValue.isJsonNull()) {
                result.remove(commonKey);
                continue;
            }

            JsonElement baseValue = result.get(commonKey);
            if (overrideValue.isJsonObject() && baseValue.isJsonObject()) {
                doOverrideDeep(baseValue.getAsJsonObject(), overrideValue.getAsJsonObject());
                continue;
            }

            result.add(commonKey, overrideValue);
        }

        for (String newKey : newKeys) {
            JsonElement element = override.get(newKey);
            if (element.isJsonNull()) {
                continue;
            }

            result.add(newKey, element);
        }
    }

    private static JobResource popResourceByType(JobResourceType resourceType, List<JobResource> resources) {
        Iterator<JobResource> iterator = resources.iterator();
        while (iterator.hasNext()) {
            JobResource resource = iterator.next();
            if (resourceType.equals(resource.getResourceType())) {
                iterator.remove();
                return resource;
            }
        }
        throw new IllegalArgumentException("resource with type " + resourceType.getMessageName()
                + " not found in inputs");
    }

    private static List<JobResource> drainResourcesByType(JobResourceType resourceType, List<JobResource> resources) {
        Iterator<JobResource> iterator = resources.iterator();
        List<JobResource> matched = new ArrayList<>();
        while (iterator.hasNext()) {
            JobResource resource = iterator.next();
            if (resourceType.equals(resource.getResourceType())) {
                iterator.remove();
                matched.add(resource);
            }
        }
        return matched;
    }

    private static void assertOnlyOptionalResourcesLeft(Collection<JobResource> resources) {
        List<JobResource> nonOptional = resources.stream()
                .filter(r -> !r.isOptional())
                .collect(Collectors.toList());

        if (!nonOptional.isEmpty()) {
            log.error("Found non optional resource in {}", resources);
            throw new IllegalArgumentException("found non optional resources in " + resources);
        }
    }

    private static void assertNoFieldsInInputRootDataLeft(Map<String, JsonObject> inputsDataByParentField) {
        Preconditions.checkArgument(
                inputsDataByParentField.size() == 0,
                "found unused fields: %s", inputsDataByParentField
        );
    }

    private static void validateInput(Descriptor inputMetadata, JsonObject inputData) {
        // Need additional transformation before validating the input: remove all direct expressions.
        // I.e. "field: ${context.value}" means the field will be removed during validation
        // But "field: value ${context.value}" will not be removed, such expressions allowed for string types only
        try {
            var cleanInputData = PropertiesSubstitutor.cleanup(inputData);
            ProtobufSerialization.deserializeFromGson(cleanInputData, DynamicMessage.newBuilder(inputMetadata));
        } catch (InvalidProtocolBufferException | TaskUnrecoverableException ex) {
            throw new TaskletMetadataValidationException(String.format(
                    "Unable to parse protobuf input message %s: %s",
                    inputMetadata.getFullName(),
                    ex.getMessage()
            ));
        }
    }

    private static Descriptor getMessageDescriptor(TaskletDescriptor descriptor, MessageType type) {
        var resourceType = resourceType(descriptor, type);
        var message = descriptor.getMessageDescriptors().get(resourceType.getMessageName());
        if (message == null) {
            throw new TaskletMetadataValidationException(
                    "Unable to find message descriptor for %s: %s".formatted(resourceType, descriptor.getId())
            );
        }
        return message;
    }

    private List<TaskletSchema.Field> extractFields(
            TaskletDescriptor descriptor,
            MessageType messageType,
            boolean single
    ) {
        if (single) {
            var resourceType = resourceType(descriptor, messageType);
            return List.of(TaskletSchema.Field.regular(resourceType));
        }
        var messageDescriptor = getMessageDescriptor(descriptor, messageType);
        return messageDescriptor.getFields().stream()
                .map(field -> {
                    JobResourceType fieldType = getResourceTypeFromField(field, messageType);
                    if (field.isRepeated()) {
                        return TaskletSchema.Field.repeated(fieldType);
                    }
                    return TaskletSchema.Field.regular(fieldType);
                })
                .collect(Collectors.toList());
    }

    private static JobResourceType getResourceTypeFromField(FieldDescriptor field, MessageType messageType) {
        if (field.getType() != FieldDescriptor.Type.MESSAGE) {
            throw new SchemaException(
                    String.format("tasklet %s message %s cannot have primitive fields. %s is %s",
                            messageType.name().toLowerCase(),
                            field.getContainingType().getFullName(),
                            field.getName(),
                            field.getType().name().toLowerCase()
                    ));
        }
        if (field.isMapField()) {
            throw new SchemaException(
                    String.format("tasklet %s message %s cannot have map fields. %s is %s",
                            messageType.name().toLowerCase(),
                            field.getContainingType().getFullName(),
                            field.getName(),
                            "map<>"
                    ));
        }
        return JobResourceType.ofField(field);
    }

    private static JobResourceType resourceType(TaskletDescriptor descriptor, MessageType type) {
        return switch (type) {
            case INPUT -> descriptor.getInputType();
            case OUTPUT -> descriptor.getOutputType();
        };
    }

    private enum MessageType {
        INPUT, OUTPUT
    }

}
