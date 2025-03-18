package ru.yandex.ci.core.resolver;

import java.util.Map;
import java.util.function.Function;

import javax.annotation.Nullable;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import com.google.protobuf.Message;

import ru.yandex.ci.core.proto.ProtobufSerialization;

public class PropertiesSubstitutor {
    public static final String TASKS_KEY = "tasks";
    public static final String CONTEXT_KEY = "context";
    public static final String FLOW_VARS_KEY = "flow-vars";

    private static final JmesPathResolver JMES_PATH_RESOLVER = new JmesPathResolver();

    private PropertiesSubstitutor() {
    }

    public static JsonObject asJsonObject(Message properties) {
        return ProtobufSerialization.serializeToGson(properties);
    }

    public static JsonObject asTasks(Map<String, JsonObject> taskMap) {
        var tasks = new JsonObject();
        for (var e : taskMap.entrySet()) {
            tasks.add(e.getKey(), e.getValue());
        }

        var json = new JsonObject();
        json.add(PropertiesSubstitutor.TASKS_KEY, tasks);

        return json;
    }

    /**
     * Check if provided element could be substituted later
     *
     * @param source element
     * @return true if this is a string with substitutions
     */
    public static boolean hasStringSubstitution(JsonElement source) {
        if (!source.isJsonPrimitive()) {
            return false;
        }
        var sourcePrimitive = source.getAsJsonPrimitive();
        return sourcePrimitive.isString() && JMES_PATH_RESOLVER.hasSubstitution(sourcePrimitive.getAsString());
    }

    /**
     * Values substitution in provided source
     *
     * @param source   object to evaluate and substitute (this object won't change)
     * @param document the document source we should lookup during object evaluation
     * @return copy of source object with resolved variables
     */
    public static JsonElement substitute(JsonElement source, DocumentSource document) {
        return doSubstitute(null, source, JMES_PATH_RESOLVER.lookup(document, false));
    }

    /**
     * Simple substitution into string
     *
     * @param expression expression to evaluate
     * @param document   document source
     * @param field      name of field we're evaluating
     * @return resolved string
     */
    public static String substituteToString(String expression, DocumentSource document, String field) {
        var element = substitute(new JsonPrimitive(expression), document);
        if (element.isJsonPrimitive()) {
            var primitive = element.getAsJsonPrimitive();
            if (primitive.isString()) {
                return primitive.getAsString();
            }
        }

        throw new IllegalStateException("Unable to calculate " + field + " expression: " + expression +
                ", must be string, got " + element);
    }

    /**
     * Simple substitution into string or number
     *
     * @param expression expression to evaluate
     * @param document   document source
     * @param field      name of field we're evaluating
     * @return resolved string
     */
    public static String substituteToStringOrNumber(String expression, DocumentSource document, String field) {
        var element = substitute(new JsonPrimitive(expression), document);
        if (element.isJsonPrimitive()) {
            var primitive = element.getAsJsonPrimitive();
            if (primitive.isString()) {
                return primitive.getAsString();
            } else if (primitive.isNumber()) {
                return primitive.getAsString();
            }
        }

        throw new IllegalStateException("Unable to calculate " + field + " expression: " + expression +
                ", must be string or number, got " + element);
    }

    /**
     * Simple substitution into number
     *
     * @param expression expression to evaluate
     * @param document   document source
     * @param field      name of field we're evaluating
     * @return resolved string
     */
    public static long substituteToNumber(String expression, DocumentSource document, String field) {
        var element = substitute(new JsonPrimitive(expression), document);
        if (element.isJsonPrimitive()) {
            try {
                var primitive = element.getAsJsonPrimitive();
                if (primitive.isString()) {
                    return primitive.getAsLong();
                } else if (primitive.isNumber()) {
                    return primitive.getAsLong();
                }
            } catch (NumberFormatException nfe) {
                //
            }
        }
        throw new IllegalStateException("Unable to calculate " + field + " expression: " + expression +
                ", must be number, got " + element);
    }

    public static boolean asBoolean(JsonElement result) {
        if (result.isJsonPrimitive()) {
            var resultPrimitive = result.getAsJsonPrimitive();
            if (resultPrimitive.isNumber()) {
                return resultPrimitive.getAsNumber().intValue() > 0;
            } else {
                return resultPrimitive.getAsBoolean();
            }
        } else if (result.isJsonObject()) {
            return result.getAsJsonObject().size() > 0;
        } else if (result.isJsonArray()) {
            return result.getAsJsonArray().size() > 0;
        } else {
            return false;
        }
    }

    /**
     * Simple cleanup for expression
     *
     * @param expression expression to cleanup
     * @param field      name of field we're evaluating
     * @return cleaned string
     */
    @Nullable
    public static String cleanupString(String expression, String field) {
        var element = cleanup(new JsonPrimitive(expression));
        if (element.isJsonPrimitive()) {
            var primitive = element.getAsJsonPrimitive();
            if (primitive.isString()) {
                return primitive.getAsString();
            }
        } else if (element.isJsonNull()) {
            return null;
        }
        throw new IllegalStateException("Unable to cleanup " + field + " expression: " + expression +
                ", must be string, got " + element);
    }

    /**
     * Values cleanup with default configuration
     *
     * @param source object to evaluate and clean up
     * @return copy of source object
     * @see #cleanup(JsonElement, DocumentSource)
     */
    public static JsonElement cleanup(JsonElement source) {
        return cleanup(source, DocumentSource.of(JsonObject::new, JsonObject::new));
    }

    /**
     * Values cleanup in provided source
     *
     * @param source   object to evaluate and clean up (all expressions will be replaced to empty strings, all
     *                 full replacements will be removed)
     * @param document document configuration
     * @return copy of source object with cleaned variables
     */
    public static JsonElement cleanup(JsonElement source, DocumentSource document) {
        return doSubstitute(null, source, JMES_PATH_RESOLVER.lookup(document, true));
    }


    private static JsonElement doSubstitute(
            @Nullable String field,
            JsonElement source,
            Function<String, JsonElement> substitutor
    ) {
        if (source.isJsonPrimitive()) {
            JsonPrimitive primitive = source.getAsJsonPrimitive();
            if (!primitive.isString()) {
                return primitive;
            }
            var value = primitive.getAsString();
            try {
                return substitutor.apply(value);
            } catch (RuntimeException e) {
                var prefix = field != null ? " field \"%s\"".formatted(field) : "";
                throw new SubstitutionRuntimeException("Unable to substitute%s with expression \"%s\": %s".formatted(
                        prefix, value, e.getMessage()), e);
            }
        } else if (source.isJsonNull()) {
            return source;
        } else if (source.isJsonArray()) {
            JsonArray sourceArray = source.getAsJsonArray();
            JsonArray targetArray = new JsonArray(sourceArray.size());
            for (JsonElement element : sourceArray) {
                var result = doSubstitute(field, element, substitutor);
                if (!result.isJsonNull()) {
                    targetArray.add(result);
                }
            }
            return targetArray;
        } else if (source.isJsonObject()) {
            JsonObject sourceObject = source.getAsJsonObject();
            JsonObject targetObject = new JsonObject();
            for (var entry : sourceObject.entrySet()) {
                var key = transformKey(field, entry.getKey(), substitutor);
                if (key != null) {
                    var value = entry.getValue();
                    var complexKey = field == null ? key : (field + "." + key);
                    var valueResult = doSubstitute(complexKey, value, substitutor);
                    if (!valueResult.isJsonNull()) {
                        targetObject.add(key, valueResult);
                    }
                }
            }
            return targetObject;
        }

        throw new IllegalArgumentException("unexpected node type: " + source);
    }

    @Nullable
    private static String transformKey(
            @Nullable String field,
            String key,
            Function<String, JsonElement> substitutor
    ) {
        var complexKey = field == null ? key : (field + "." + key);
        var keyResult = doSubstitute(complexKey, new JsonPrimitive(key), substitutor);
        if (keyResult.isJsonNull()) {
            return null;
        } else {
            if (keyResult.isJsonPrimitive()) {
                return keyResult.getAsString();
            } else {
                throw new IllegalStateException("Unable to calculate " + complexKey + " expression: " + key +
                        ", must be string, got " + keyResult);
            }
        }
    }

}
