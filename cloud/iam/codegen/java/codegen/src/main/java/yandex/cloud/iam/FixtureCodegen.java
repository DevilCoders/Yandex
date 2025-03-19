package yandex.cloud.iam;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

import com.beust.jcommander.JCommander;
import com.beust.jcommander.Parameter;
import com.beust.jcommander.converters.FileConverter;
import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.dataformat.yaml.YAMLFactory;
import com.fasterxml.jackson.dataformat.yaml.YAMLGenerator;
import com.google.common.base.CaseFormat;
import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.Value;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;
import org.springframework.core.io.support.ResourcePatternResolver;

public class FixtureCodegen {
    private static final String SYSTEM_SERVICE_NAME = "system";

    @Parameter(names = "--output-dir", required = true, converter = FileConverter.class)
    private File outputDir;
    @Parameter(names = "--model", required = false, converter = FileConverter.class)
    private File model;
    @Parameter(names = "--package", required = false)
    private String packageName = "yandex.cloud.codegen";

    public static void main(String... args) throws IOException {
        FixtureCodegen tool = new FixtureCodegen();
        JCommander commander = new JCommander(tool);
        commander.parse(args);
        tool.run();
    }

    private URL loadModel() {
        if (model == null) {
            ResourcePatternResolver patternResolver = new PathMatchingResourcePatternResolver();
            try {
                return patternResolver.getResource("classpath:yandex/cloud/iam/fixture_permissions.yaml")
                        .getURL();
            } catch (IOException e) {
                throw new RuntimeException("failed to load fixture_permission.yaml from classpath", e);
            }
        } else {
            try {
                return model.toURI().toURL();
            } catch (MalformedURLException e) {
                throw new RuntimeException("malformed URL for model file", e);
            }
        }
    }

    private void renderServices(Fixtures fixtures, File packageDir) throws IOException {
        List<Service> services = fixtures.getServices().stream()
                .map(Service::new)
                .collect(Collectors.toList());
        services.add(Service.SYSTEM);

        String content = FreemarkerProcessor.processFreeMarkerTemplate("service.ftl",
                Map.of("services", services,
                        "package", packageName,
                        "enum_name", "Service"));
        try (FileWriter writer = new FileWriter(new File(packageDir, "Service.java"))) {
            writer.append(content);
        }
    }

    private void renderQuotas(Fixtures fixtures, File packageDir) throws IOException {
        Map<String, List<Quota>> serviceQuotas = new HashMap<>();
        fixtures.getQuotas().forEach(quotaName -> {
            Quota quota = Quota.build(quotaName);
            serviceQuotas.putIfAbsent(quota.getServiceName(), new ArrayList<>());
            serviceQuotas.get(quota.getServiceName()).add(quota);
        });

        String perServiceContent = FreemarkerProcessor.processFreeMarkerTemplate("quotas.ftl",
                Map.of("service_quotas", serviceQuotas,
                        "package", packageName,
                        "global_enum_name", "Quota",
                        "class_name", "Quotas"));
        try (FileWriter writer = new FileWriter(new File(packageDir, "Quotas.java"))) {
            writer.append(perServiceContent);
        }

        Map<String, Service> services = collectServices(fixtures);
        List<Quota> allQuotas = serviceQuotas.values().stream().flatMap(Collection::stream)
                .sorted(Comparator.comparing(Quota::getName))
                .collect(Collectors.toList());
        String content = FreemarkerProcessor.processFreeMarkerTemplate("quota.ftl",
                Map.of("quotas", allQuotas,
                        "package", packageName,
                        "enum_name", "Quota",
                        "services", services));
        try (FileWriter writer = new FileWriter(new File(packageDir, "Quota.java"))) {
            writer.append(content);
        }
    }

    private void renderResources(Fixtures fixtures, File packageDir) throws IOException {
        Map<String, List<ResourceType>> serviceResources = new HashMap<>();
        fixtures.getResources().forEach(resourceName -> {
            ResourceType resourceType = ResourceType.build(resourceName);
            serviceResources.putIfAbsent(resourceType.getServiceName(), new ArrayList<>());
            serviceResources.get(resourceType.getServiceName()).add(resourceType);
        });

        String perServiceContent = FreemarkerProcessor.processFreeMarkerTemplate("resource_types.ftl",
                Map.of("service_resources", serviceResources,
                        "package", packageName,
                        "global_enum_name", "ResourceType",
                        "class_name", "ResourceTypes"));
        try (FileWriter writer = new FileWriter(new File(packageDir, "ResourceTypes.java"))) {
            writer.append(perServiceContent);
        }

        Map<String, Service> services = collectServices(fixtures);
        List<ResourceType> allResources = serviceResources.values().stream().flatMap(Collection::stream)
                .sorted(Comparator.comparing(ResourceType::getName))
                .collect(Collectors.toList());
        String content = FreemarkerProcessor.processFreeMarkerTemplate("resource_type.ftl",
                Map.of("resources", allResources,
                        "package", packageName,
                        "enum_name", "ResourceType",
                        "services", services));
        try (FileWriter writer = new FileWriter(new File(packageDir, "ResourceType.java"))) {
            writer.append(content);
        }
    }

    private void renderPermissions(Fixtures fixtures, File packageDir) throws IOException {
        Map<String, List<Permission>> servicePermissions = new HashMap<>();
        Set<String> serviceNames = new HashSet<>();
        fixtures.getServices().forEach(service -> {
            serviceNames.add(service.getId());
            if (service.getAliases() != null) {
                serviceNames.addAll(service.getAliases());
            }
        });
        fixtures.getPermissions().forEach(permissionName -> {
            Permission permission = Permission.build(permissionName, serviceNames);
            servicePermissions.putIfAbsent(permission.getServiceName(), new ArrayList<>());
            servicePermissions.get(permission.getServiceName()).add(permission);
        });

        for (Map.Entry<String, List<Permission>> entry : servicePermissions.entrySet()) {
            String serviceName = entry.getKey();
            String className = entry.getValue().get(0).getService().camelUpperCase();
            String subPackageName = "permission";
            String content = FreemarkerProcessor.processFreeMarkerTemplate("permissions.ftl",
                    Map.of("class_name", className,
                            "package", packageName + "." + subPackageName,
                            "permissions", entry.getValue()));
            File subPackageFile = Paths.get(packageDir.getAbsolutePath(), subPackageName).toFile();
            subPackageFile.mkdirs();
            try (FileWriter writer = new FileWriter(new File(subPackageFile, className + ".java"))) {
                writer.append(content);
            }
        }
    }

    private void renderRestrictionKinds(Fixtures fixtures, File packageDir) throws IOException {
        List<RestrictionKind> restrictionKinds = fixtures.getRestrictionKinds().stream()
                .map(RestrictionKind::build)
                .collect(Collectors.toList());

        String content = FreemarkerProcessor.processFreeMarkerTemplate("restriction_kind.ftl",
                Map.of("restriction_kinds", restrictionKinds,
                        "package", packageName,
                        "enum_name", "RestrictionKind"));
        try (FileWriter writer = new FileWriter(new File(packageDir, "RestrictionKind.java"))) {
            writer.append(content);
        }
    }

    private void renderRestrictionTypes(Fixtures fixtures, File packageDir) throws IOException {
        List<RestrictionType> restrictionTypes = fixtures.getRestrictions().stream()
                .map(RestrictionType::build)
                .collect(Collectors.toList());

        String content = FreemarkerProcessor.processFreeMarkerTemplate("restriction_type.ftl",
                Map.of("restrictionTypes", restrictionTypes,
                        "package", packageName,
                        "enum_name", "RestrictionType",
                        "kind_enum_name", "RestrictionKind"));
        try (FileWriter writer = new FileWriter(new File(packageDir, "RestrictionType.java"))) {
            writer.append(content);
        }
    }

    private Map<String, Service> collectServices(Fixtures fixtures) {
        Map<String, Service> services = new HashMap<>();

        services.put(Service.SYSTEM.getServiceName(), Service.SYSTEM);
        for (Fixtures.Service fixtureService : fixtures.getServices()) {
            var service = new Service(fixtureService);

            services.put(fixtureService.getId(), service);
            for (String alias : service.getAliases()) {
                services.put(alias, service);
            }
        }

        return services;
    }

    public void run() throws IOException {
        YAMLFactory yamlFactory = new YAMLFactory();
        yamlFactory.disable(YAMLGenerator.Feature.CANONICAL_OUTPUT);
        yamlFactory.disable(YAMLGenerator.Feature.INDENT_ARRAYS);
        ObjectMapper objectMapper = new ObjectMapper(yamlFactory);
        objectMapper.configure(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES, false);
        Fixtures fixtures = objectMapper.readValue(loadModel(), Fixtures.class);

        File packageDir = Paths.get(outputDir.getAbsolutePath(), packageName.replace(".", "/")).toFile();
        packageDir.mkdirs();

        renderServices(fixtures, packageDir);
        renderQuotas(fixtures, packageDir);
        renderResources(fixtures, packageDir);
        renderPermissions(fixtures, packageDir);
        renderRestrictionKinds(fixtures, packageDir);
        renderRestrictionTypes(fixtures, packageDir);
    }

    @Value
    public static class Service {
        public static final Service SYSTEM = new Service(SYSTEM_SERVICE_NAME, List.of());

        private final String serviceName;
        private final List<String> aliases;
        private final Identifier service;

        public Service(String serviceName, List<String> aliases) {
            this.serviceName = serviceName;
            this.aliases = List.copyOf(aliases);
            this.service = Identifier.of(serviceName.split("-"));
        }

        public Service(Fixtures.Service service) {
            this(service.getId(), service.getAliases());
        }
    }

    @Value
    public static class Quota {
        private final String serviceName;
        private final String name;
        private final Identifier service;
        private final Identifier type;

        private static final Pattern QUOTA_NAME_REGEX = Pattern.compile("(?<service>[a-zA-Z0-9-]+)\\." +
                "(?<quotaType>[a-zA-Z0-9-]+)\\.(?<metricType>.+)");

        public static Quota build(String quotaName) {
            Matcher matcher = QUOTA_NAME_REGEX.matcher(quotaName);
            if (!matcher.matches()) {
                throw new RuntimeException("invalid quota name " + quotaName);
            }
            String serviceName = matcher.group("service");
            Identifier service = Identifier.of(serviceName.split("-"));

            String quotaType = matcher.group("quotaType").replace("-", "_");
            String metricType = matcher.group("metricType");

            List<String> parts = Arrays
                    .stream(CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, quotaType).split("_"))
                    .collect(Collectors.toList());
            parts.add(metricType);
            return new Quota(serviceName, quotaName, service, new Identifier(parts));
        }
    }

    @Value
    public static class Permission {
        private final String serviceName;
        private final String name;
        private final Identifier service;
        private final Identifier type;

        private static final Pattern PERMISSION_NAME_REGEX = Pattern.compile("(?<service>[a-zA-Z0-9-]+)\\.(?<suffix>[a-zA-Z0-9-.]+)");

        public static Permission build(String permission, Set<String> serviceNames) {
            Matcher matcher = PERMISSION_NAME_REGEX.matcher(permission);
            if (!matcher.matches()) {
                throw new RuntimeException("invalid permission name " + permission);
            }
            String serviceName = matcher.group("service").toLowerCase();
            String suffix;
            if (!serviceNames.contains(serviceName)) {
                serviceName = "without-service";
                suffix = permission;
            } else {
                suffix = matcher.group("suffix").replace("-", "_");
            }
            Identifier service = Identifier.of(serviceName.split("-"));

            List<String> parts = Arrays.stream(CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, suffix).split(
                    "[-_\\.]"))
                    .collect(Collectors.toList());
            return new Permission(serviceName, permission, service, new Identifier(parts));
        }
    }

    @Value
    public static class ResourceType {
        private final String serviceName;
        private final String name;
        private final Identifier service;
        private final Identifier type;

        private static final Pattern RESOURCE_NAME_REGEX = Pattern.compile("(?<system>root|resourceType)|" +
                "(?<service>[a-zA-Z0-9-]+)\\.(?<suffix>[a-zA-Z-]+)");

        public static ResourceType build(String resourceName) {
            Matcher matcher = RESOURCE_NAME_REGEX.matcher(resourceName);
            if (!matcher.matches()) {
                throw new RuntimeException("invalid resource name " + resourceName);
            }
            String serviceName = matcher.group("service");
            String suffix;
            if (serviceName == null) {
                serviceName = SYSTEM_SERVICE_NAME;
                suffix = matcher.group("system");
            } else {
                suffix = matcher.group("suffix").replace("-", "_");
            }
            Identifier service = Identifier.of(serviceName.split("-"));

            List<String> parts = Arrays.stream(CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, suffix).split(
                    "_"))
                    .collect(Collectors.toList());
            return new ResourceType(serviceName, resourceName, service, new Identifier(parts));
        }
    }

    @Value
    public static class RestrictionKind {
        private final String name;
        private final Identifier restrictionKind;

        public static RestrictionKind build(String restrictionKind) {
            List<String> restrictionKindParts = Arrays
                    .stream(CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, restrictionKind).split("[-_]"))
                    .collect(Collectors.toList());

            return new RestrictionKind(restrictionKind, new Identifier(restrictionKindParts));
        }
    }

    @Value
    public static class RestrictionType {
        private static final Pattern RESTRICTION_TYPE_NAME_REGEX =
            Pattern.compile("(?<kind>[a-zA-Z0-9-]+)\\.(?<suffix>[a-zA-Z0-9-.]+)");

        private final String name;
        private final Identifier kind;
        private final Identifier type;

        public static RestrictionType build(String restrictionType) {
            Matcher matcher = RESTRICTION_TYPE_NAME_REGEX.matcher(restrictionType);
            if (!matcher.matches()) {
                throw new RuntimeException("invalid restriction type name " + restrictionType);
            }

            List<String> restrictionKindParts = Arrays
                    .stream(CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, matcher.group("kind")).split("[-_]"))
                    .collect(Collectors.toList());

            List<String> restrictionTypeParts = Arrays
                    .stream(CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, matcher.group("suffix")).split("[-_]"))
                    .collect(Collectors.toList());

            return new RestrictionType(restrictionType, new Identifier(restrictionKindParts), new Identifier(restrictionTypeParts));
        }
    }

    @Value
    public static class Identifier {
        private List<String> parts;

        public static Identifier of(String... parts) {
            return new Identifier(Arrays.stream(parts).collect(Collectors.toList()));
        }

        public String snakeUpperCase() {
            return parts.stream().map(String::toUpperCase).collect(Collectors.joining("_"));
        }

        public String camelUpperCase() {
            return parts.stream().map(part -> CaseFormat.LOWER_CAMEL.to(CaseFormat.UPPER_CAMEL, part))
                    .collect(Collectors.joining());
        }
    }

    @Getter
    @EqualsAndHashCode
    private static class Fixtures {
        List<String> quotas;
        List<String> resources;
        List<String> permissions;
        List<Service> services;
        List<String> restrictionKinds;
        List<String> restrictions;

        @Getter
        @EqualsAndHashCode
        private static class Service {
            String id;
            List<String> aliases;
        }
    }
}
