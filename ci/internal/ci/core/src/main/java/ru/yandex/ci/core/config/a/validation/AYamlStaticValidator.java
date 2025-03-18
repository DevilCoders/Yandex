package ru.yandex.ci.core.config.a.validation;

import java.nio.file.Path;
import java.util.List;
import java.util.Set;
import java.util.function.BiConsumer;
import java.util.function.Consumer;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.databind.JsonNode;
import com.google.common.base.Strings;
import com.google.common.collect.Sets;
import com.google.gson.JsonObject;
import lombok.AllArgsConstructor;
import lombok.Value;
import one.util.streamex.StreamEx;
import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.config.Validation;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.model.ActionConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.FilterConfig;
import ru.yandex.ci.core.config.a.model.FlowConfig;
import ru.yandex.ci.core.config.a.model.FlowVarsUi;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.core.config.a.model.TriggerConfig;
import ru.yandex.ci.core.launch.FlowVarsService;
import ru.yandex.ci.core.schema.JsonSchemaUtils;
import ru.yandex.ci.util.jackson.parse.ParseInfo;

/**
 * Валидирует конфиг a.yaml статически, без использования внешних знаний.
 */
@AllArgsConstructor
public class AYamlStaticValidator {

    public static final String GLOB_CHARS = "*?[]{}";

    @Nonnull
    private final AYamlConfig aYaml;

    @Nullable
    private ArcCommit commit;

    public Set<String> validate() {
        var ci = aYaml.getCi();
        if (ci == null) {
            return Set.of();
        }
        return new CiConfigValidator(ci, commit).validate();
    }

    @AllArgsConstructor
    private static class CiConfigValidator {

        @Nonnull
        private final CiConfig ci;

        @Nullable
        private ArcCommit commit;

        private final ValidationErrors errors = new ValidationErrors();

        Set<String> validate() {
            new AYamlJobsDependenciesValidator(ci, errors).validate();
            boolean noDepsErrors = !errors.hasErrors();

            validateRuntime();
            validateNonEmptyFlows();
            validateActions();
            validateTriggers();
            validateActionsAndTriggersMix();
            validateAuto();
            validateFilters();
            validateFlowVars();
            if (noDepsErrors) {
                //Do not validate stages if deps are invalid
                new AYamlStageValidator(ci, errors).validate();
            }
            return errors.getErrors();
        }

        private void validateRuntime() {
            if (ci.getRuntime() == null) {
                return;
            }
            var sandboxConfig = ci.getRuntime().getSandbox();
            // не валидируем схему, потому что может использоваться в переопределении,
            // где мы позволяем не указывать owner
            if (Strings.isNullOrEmpty(sandboxConfig.getOwner())) {
                errors.add("ci.runtime must have sandbox.owner or sandbox-owner property");
            }
        }

        private void validateNonEmptyFlows() {
            for (FlowConfig flow : ci.getFlows().values()) {
                if (flow.getJobs().isEmpty()) {
                    errors.add(String.format(
                            "flow at %s must have at least one job",
                            flow.getParseInfo().getParsedPath()
                    ));
                }
            }
        }

        private void validateAuto() {
            for (ReleaseConfig release : ci.getReleases().values()) {
                if (release.getAuto().isEnabled() && release.getBranches().isForbidTrunkReleases()) {
                    errors.add("release at %s has enabled auto but %s is true"
                            .formatted(
                                    release.getParseInfo().getParsedPath(),
                                    release.getBranches().getParseInfo().getParsedPath("forbid-trunk-releases")
                            )
                    );
                }

                Stream.concat(
                        release.getAuto().getConditions().stream(),
                        release.getBranches().getAuto().getConditions().stream()
                ).forEach(
                        condition -> {
                            if (condition.getMinCommits() != null
                                    && condition.getMinCommits() == 0
                                    && condition.getSinceLastRelease() == null
                            ) {
                                errors.add("cannot have min-commits: 0 with undefined since-last-release");
                            }
                        }
                );
            }
        }

        private void validateActionsAndTriggersMix() {
            if (!ci.getActions().isEmpty() && !ci.getTriggers().isEmpty()) {
                var triggerFlows = ci.getTriggers().stream()
                        .map(TriggerConfig::getFlow)
                        .collect(Collectors.toSet());
                for (var action : ci.getActions().values()) {
                    var id = action.getId();
                    if (triggerFlows.contains(id)) {
                        errors.add(String.format(
                                "Cannot have action with id %s and trigger using flow with same id, " +
                                        "please migrate to Actions completely", id));
                    }
                }
            }
        }

        private void validateActions() {
            for (var action : ci.getActions().values()) {
                validateTriggersImpl(action.getTriggers());
            }
        }

        private void validateTriggers() {
            validateTriggersImpl(ci.getTriggers());
        }

        private void validateTriggersImpl(List<TriggerConfig> triggers) {
            for (TriggerConfig trigger : triggers) {
                if (trigger.getOn() != TriggerConfig.On.PR) {
                    BiConsumer<String, String> addError = (path, field) ->
                            errors.add(
                                    String.format(
                                            "trigger at %s has '%s', but '%s' option is not a %s",
                                            path,
                                            field,
                                            "on",
                                            TriggerConfig.On.PR.name()
                                    )
                            );

                    if (trigger.getRequired() != null) {
                        addError.accept(trigger.getParseInfo().getParsedPath(), "required");
                    }
                    for (var filter : trigger.getFilters()) {
                        if (!filter.getFeatureBranches().isEmpty()) {
                            // ParseInfo does not work on @Value objects?
                            addError.accept(trigger.getParseInfo().getParsedPath() + "/filters",
                                    "feature-branches");
                        }
                    }
                }
            }
        }

        private void validateFilters() {
            for (var action : ci.getActions().values()) {
                for (var trigger : action.getTriggers()) {
                    validateFiltersImpl(trigger.getOn(), trigger.getFilters(), trigger.getParseInfo());
                }
            }
            for (var trigger : ci.getTriggers()) {
                validateFiltersImpl(trigger.getOn(), trigger.getFilters(), trigger.getParseInfo());
            }
            for (var release : ci.getReleases().values()) {
                validateFiltersImpl(null, release.getFilters(), release.getParseInfo());
            }
        }

        private void validateFiltersImpl(@Nullable TriggerConfig.On on,
                                         List<FilterConfig> filters,
                                         ParseInfo parseInfo) {
            for (var filter : filters) {
                var discovery = filter.getDiscovery();

                if (on == TriggerConfig.On.PR && !filter.getAbsPaths().isEmpty()) {
                    if (discovery == FilterConfig.Discovery.GRAPH) {
                        errors.add(String.format(
                                "trigger on %s does not support %s discovery [%s]",
                                on,
                                FilterConfig.Discovery.GRAPH,
                                parseInfo.getParsedPath()));
                    } else {
                        errors.add(String.format(
                                "trigger on %s does not support abs-paths [%s]",
                                on,
                                parseInfo.getParsedPath()));
                    }
                    continue; // --- no additional checks required
                }

                for (var subPath : filter.getSubPaths()) {
                    validatePath(parseInfo, "sub-paths", subPath);
                }

                for (var absPath : filter.getAbsPaths()) {
                    if (!validatePath(parseInfo, "abs-paths", absPath)) {
                        continue;
                    }

                    if (discovery != FilterConfig.Discovery.GRAPH) {
                        Consumer<String> addError = msg -> addPathError(parseInfo, "abs-paths", absPath, msg);
                        var actualPath = Path.of(absPath);
                        if (actualPath.getNameCount() == 0) {
                            addError.accept("Path cannot be empty");
                            continue;
                        }
                        var firstIndex = actualPath.getName(0).toString();
                        if (StringUtils.containsAny(firstIndex, GLOB_CHARS)) {
                            addError.accept("Path must contains at least one top-level folder " +
                                    "and cannot have wildcard chars: " + GLOB_CHARS);
                        }
                    }
                }
            }
        }

        private void addPathError(ParseInfo parseInfo, String srcPathType, String srcPath, String msg) {
            errors.add(String.format(
                    "filter at %s has has invalid %s configuration [%s]: %s",
                    parseInfo.getParsedPath(),
                    srcPathType,
                    srcPath,
                    msg));
        }

        private boolean validatePath(ParseInfo parseInfo, String srcPathType, String srcPath) {
            Consumer<String> addError = msg -> addPathError(parseInfo, srcPathType, srcPath, msg);

            // Make sure this expression could be used for discovery
            var path = srcPath.trim();
            if (path.startsWith("/") || path.startsWith("\\")) {
                addError.accept("Path cannot starts from / or \\");
                return false;
            }
            if (path.startsWith("..")) {
                addError.accept("Path cannot be relative");
                return false;
            }
            if (path.isEmpty()) {
                addError.accept("Path cannot be empty");
                return false;
            }
            return true;
        }

        private void validateFlowVars() {
            var fromReleases = ci.getReleases().values()
                    .stream()
                    .flatMap(r -> Stream.concat(
                                    Stream.of(FlowVarsAndSchema.of(r.getFlowVarsUi(), r.getFlowVars())),
                                    Stream.concat(r.getHotfixFlows().stream(), r.getRollbackFlows().stream())
                                            .map(f -> FlowVarsAndSchema.of(f.getFlowVarsUi(), f.getFlowVars()))
                            )
                    );

            var fromActions = ci.getActions().values()
                    .stream()
                    .map(a -> FlowVarsAndSchema.of(a.getFlowVarsUi(), a.getFlowVars()));

            Stream.concat(fromReleases, fromActions)
                    .forEach(this::validateFlowVarsUi);

            ci.getReleases().values()
                    .forEach(this::validateFlowVarsForReleases);

            ci.getActions().values()
                    .forEach(this::validateFlowVarsForActions);
        }

        private void validateFlowVarsForReleases(ReleaseConfig config) {
            if (!config.getAuto().isEnabled() && !config.getBranches().getAuto().isEnabled()) {
                return;
            }

            validateFlowVarsUiDefaults(config.getFlowVarsUi(),
                    "Release %s has enabled auto, but some flow-vars-ui top-level fields don't have defaults"
                            .formatted(config.getId())
            );
        }

        private void validateFlowVarsForActions(ActionConfig config) {
            if (config.getTriggers().isEmpty()) {
                return;
            }

            validateFlowVarsUiDefaults(config.getFlowVarsUi(),
                    "Action %s has trigger, but some flow-vars-ui top-level fields don't have defaults"
                            .formatted(config.getId())
            );
        }

        private void validateFlowVarsUiDefaults(@Nullable FlowVarsUi flowVarsUi, String message) {
            if (flowVarsUi == null) {
                return;
            }

            var schema = flowVarsUi.getSchema();
            var fieldsWithoutDefaults = FlowVarsService.getFieldsWithoutDefaults(schema);
            if (!fieldsWithoutDefaults.isEmpty()) {
                errors.add(
                        "%s: %s.".formatted(message, String.join(", ", fieldsWithoutDefaults))
                );
            }
        }

        private void validateFlowVarsUi(FlowVarsAndSchema flowVarsAndSchema) {
            var declaredFlowVars = flowVarsAndSchema.getDeclaredFlowVars();
            var ui = flowVarsAndSchema.getUi();
            if (ui == null) {
                return;
            }

            var jsonSchema = ui.getSchema();
            if (Validation.applySince(commit, 9722519)) {
                validatePropertyTypes(jsonSchema);
            }

            if (declaredFlowVars == null) {
                return;
            }
            validateFlowVarsDuplicateDeclaration(declaredFlowVars, jsonSchema);

        }

        private void validateFlowVarsDuplicateDeclaration(JsonObject declaredFlowVars, JsonNode jsonSchema) {
            var declaredKeys = declaredFlowVars.keySet();
            var inSchemaKeys = StreamEx.of(jsonSchema.path("properties").fieldNames()).toSet();

            var declaredInBothPlaces = Sets.intersection(declaredKeys, inSchemaKeys);
            if (!declaredInBothPlaces.isEmpty()) {
                errors.add(("Properties %s declared in flow-vars and flow-vars-ui.schema.properties." +
                        " Flow var value can either be static and declared in flow-vars section" +
                        " or be configurable in ui and described in flow-var-ui schema." +
                        " If you want configurable flow var with default value set `default` property in schema.")
                        .formatted(declaredInBothPlaces));
            }
        }

        private void validatePropertyTypes(JsonNode jsonSchema) {
            var propertyValidator = new JsonSchemaUtils.PropertiesVisitor() {
                private void inconsistentType(JsonNode defaultValue, JsonNode property) {
                    errors.add(("Inconsistent type in json schema." +
                            " Property type is %s, but default value %s is not a %1$s: %s")
                            .formatted(
                                    property.path("type").asText(),
                                    defaultValue.toPrettyString(),
                                    property.toString()
                            )
                    );
                }

                @Override
                public void visit(JsonNode property) {
                    var defaultValue = property.path("default");
                    if (defaultValue.isMissingNode()) {
                        return;
                    }
                    var type = property.get("type");
                    switch (type.asText()) {
                        case "boolean" -> {
                            if (!defaultValue.isBoolean()) {
                                inconsistentType(defaultValue, property);
                            }
                        }
                        case "object" -> {
                            if (!defaultValue.isObject()) {
                                inconsistentType(defaultValue, property);
                            }
                        }
                        case "array" -> {
                            if (!defaultValue.isArray()) {
                                inconsistentType(defaultValue, property);
                            }
                        }
                        case "string" -> {
                            if (!defaultValue.isTextual()) {
                                inconsistentType(defaultValue, property);
                            }
                        }
                        case "number" -> {
                            if (!defaultValue.isNumber()) {
                                inconsistentType(defaultValue, property);
                            }
                        }
                        case "integer" -> {
                            if (!defaultValue.isInt()) {
                                inconsistentType(defaultValue, property);
                            }
                        }
                        default -> { /* do nothing */ }
                    }
                }
            };

            JsonSchemaUtils.walkProperties(jsonSchema, propertyValidator);
        }

        @Value(staticConstructor = "of")
        private static class FlowVarsAndSchema {
            @Nullable
            FlowVarsUi ui;
            @Nullable
            JsonObject declaredFlowVars;
        }
    }
}
