package ru.yandex.ci.core.tasklet;

import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Function;
import java.util.function.Supplier;

import com.google.common.base.Suppliers;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorSet;
import com.google.protobuf.Descriptors;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.ExtensionRegistry;
import com.google.protobuf.InvalidProtocolBufferException;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.StreamEx;

@Slf4j
public class DescriptorsParser {

    private DescriptorsParser() {
    }

    public static Supplier<Map<String, Descriptor>> getDescriptorsCache(Supplier<byte[]> source) {
        return Suppliers.memoize(() -> parseDescriptors(parseDescriptorSet(source.get())));
    }

    public static FileDescriptorSet parseDescriptorSet(byte[] descriptors) {
        var registry = ExtensionRegistry.newInstance();
        try {
            return FileDescriptorSet.parseFrom(descriptors, registry);
        } catch (InvalidProtocolBufferException e) {
            throw new SchemaValidationException("Unable to parse descriptors from bytes", e);
        }
    }

    public static Map<String, Descriptor> parseDescriptors(FileDescriptorSet descriptorSet) {
        log.info("Parsing descriptors: {} files", descriptorSet.getFileCount());

        var files = toDescriptorFiles(descriptorSet);
        var parsedFiles = new HashMap<String, FileDescriptor>();
        var checkedFiles = new HashSet<String>();

        for (var file : files.values()) {
            parseRecursive(file, files, parsedFiles, checkedFiles);
        }

        var target = new HashMap<String, Descriptor>();
        for (var file : parsedFiles.values()) {
            for (var type : file.getMessageTypes()) {
                target.put(type.getFullName(), type);
            }
        }

        log.info("Total messages parsed: {}", target.size());
        return Map.copyOf(target);
    }

    public static FileDescriptorSet optimizeDescriptors(FileDescriptorSet descriptorSet, List<String> messageTypes) {
        var files = toDescriptorFiles(descriptorSet);
        var messageTypeSet = Set.copyOf(messageTypes);
        log.info("Optimizing descriptors for messages: {}", messageTypeSet);

        var target = new LinkedHashMap<String, FileDescriptorProto>();
        for (var file : files.values()) {
            log.info("Package: {}", file.getPackage());
            for (var type : file.getMessageTypeList()) {
                var messageType = file.getPackage() + "." + type.getName();
                log.info("Checking type: {}", messageType);
                if (messageTypeSet.contains(messageType)) {
                    target.put(file.getName(), file);
                    addRecursive(file, files, target);
                }
            }
        }

        var result = FileDescriptorSet.newBuilder()
                .addAllFile(target.values())
                .build();

        log.info("Descriptors optimized, from {} messages ({} bytes) to {} messages ({} bytes)",
                descriptorSet.getFileCount(), descriptorSet.getSerializedSize(),
                result.getFileCount(), result.getSerializedSize());
        return result;
    }

    private static void addRecursive(
            FileDescriptorProto file,
            Map<String, FileDescriptorProto> sourceMap,
            Map<String, FileDescriptorProto> targetMap
    ) {
        for (var dependency : file.getDependencyList()) {
            var fileToAdd = sourceMap.get(dependency);
            if (fileToAdd == null) {
                continue; // ---
            }
            if (targetMap.put(dependency, fileToAdd) == null) {
                addRecursive(fileToAdd, sourceMap, targetMap);
            }
        }
    }

    private static void parseRecursive(
            FileDescriptorProto file,
            Map<String, FileDescriptorProto> sourceMap,
            Map<String, FileDescriptor> targetMap,
            Set<String> checkedFiles
    ) {
        for (var dependency : file.getDependencyList()) {
            var fileToParse = sourceMap.get(dependency);
            if (fileToParse == null) {
                continue; // ---
            }
            if (checkedFiles.add(dependency)) {
                parseRecursive(fileToParse, sourceMap, targetMap, checkedFiles);
            }
        }
        var name = file.getName();
        if (!targetMap.containsKey(name)) {
            targetMap.put(name, parseImpl(file, targetMap));
        }
    }

    private static FileDescriptor parseImpl(FileDescriptorProto file, Map<String, FileDescriptor> targetMap) {
        try {
            log.info("Parsing descriptor file: {}", file.getName());
            return FileDescriptor.buildFrom(file, targetMap.values().toArray(FileDescriptor[]::new));
        } catch (Descriptors.DescriptorValidationException e) {
            throw new SchemaValidationException("Unable to parse descriptors from " + file.getName(), e);
        }
    }

    private static Map<String, FileDescriptorProto> toDescriptorFiles(FileDescriptorSet descriptorSet) {
        return StreamEx.of(descriptorSet.getFileList())
                .toMap(FileDescriptorProto::getName, Function.identity());
    }
}
