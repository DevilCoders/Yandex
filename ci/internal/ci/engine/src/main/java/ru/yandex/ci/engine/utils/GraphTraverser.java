package ru.yandex.ci.engine.utils;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Function;
import java.util.stream.Collectors;

import ru.yandex.ci.core.config.a.validation.AYamlValidationException;

/**
 * Простая имплементация обхода графа. Сделана исключительно для ci-engine. Если вам необходимы изменения
 * в этом алгоритме, возможно вам подойдет больше JGraphT.
 */
public class GraphTraverser {

    private GraphTraverser() {
    }

    @SafeVarargs
    public static <Payload> List<Payload> traverse(Node<Payload>... nodes) throws AYamlValidationException {
        return traverse(List.of(nodes));
    }

    public static <Payload> List<Payload> traverse(List<? extends Node<Payload>> nodes)
            throws AYamlValidationException {
        Map<String, Node<Payload>> nodeMap = nodes.stream().collect(Collectors.toMap(Node::key, Function.identity()));
        Map<String, Payload> payloadMap = new LinkedHashMap<>();

        List<Payload> payloads = new ArrayList<>(nodes.size());
        for (Node<Payload> node : nodes) {
            payloads.add(create0(nodeMap, payloadMap, node.key(), node.key()));
        }
        return payloads;
    }

    private static <Payload> Payload create0(
            Map<String, Node<Payload>> nodes,
            Map<String, Payload> payloads,
            String current,
            String root
    ) throws AYamlValidationException {
        if (payloads.containsKey(current)) {
            Payload exists = payloads.get(current);
            if (exists == null) {
                throw new IllegalArgumentException("cycle found in job: " + current);
            }

            return exists;
        }

        payloads.put(current, null);
        Node<Payload> currentNode = nodes.get(current);
        if (currentNode == null) {
            throw new AYamlValidationException(root, "Job with name \"%s\" is not found".formatted(current));
        }

        List<String> parentKeys = currentNode.parents();
        List<Payload> parents = new ArrayList<>(parentKeys.size());
        for (String parentKey : parentKeys) {
            parents.add(create0(nodes, payloads, parentKey, root));
        }

        Payload currentPayload = currentNode.create(parents);
        payloads.put(current, currentPayload);

        return currentPayload;
    }

    public interface Node<Payload> {
        String key();

        List<String> parents();

        Payload create(List<Payload> parents) throws AYamlValidationException;
    }
}
