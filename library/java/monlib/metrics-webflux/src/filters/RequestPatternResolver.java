package ru.yandex.monlib.metrics.webflux.filters;

import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

/**
 * @author Vladimir Gordiychuk
 */
public class RequestPatternResolver {
    private final Set<String> endpoints;
    private final TrieNode trie;

    public RequestPatternResolver(Set<String> endpoints) {
        this.endpoints = endpoints.stream()
                .map(this::removeLastSlash)
                .collect(Collectors.toSet());
        this.trie = new TrieNode("", null);
        this.endpoints.forEach(endpoint -> {
            String[] values = endpoint.split("/");
            var node = trie;
            // skip first empty
            for (int index = 1; index < values.length; index++) {
                node = node.insert(values[index]);
            }
            node.setLeaf();
        });
    }

    public Optional<String> resolve(String uri) {
        uri = removeLastSlash(uri);
        if (endpoints.contains(uri)) {
            return Optional.of(uri);
        }

        String[] values = uri.split("/");
        TrieNode node = trie.match(1, values);
        if (node == null) {
            return Optional.empty();
        }
        int index = values.length - 1;
        do {
            values[index--] = node.content;
            node = node.parent;
        } while (node != null);
        return Optional.of(String.join("/", Arrays.asList(values)));
    }

    private String removeLastSlash(String str) {
        if (str.endsWith("/")) {
            return str.substring(0, str.length() - 1);
        }

        return str;
    }

    private static class TrieNode {
        private final Map<String, TrieNode> children;
        private final Map<String, TrieNode> pathArgNodes;
        private final String content;
        private boolean leaf;
        @Nullable
        private final TrieNode parent;

        public TrieNode(String content, TrieNode parent) {
            this.children = new HashMap<>();
            this.pathArgNodes = new HashMap<>();
            this.content = content;
            this.parent = parent;
        }

        void setLeaf() {
            this.leaf = true;
        }

        public TrieNode insert(String value) {
            if (value.contains("{")) {
                var content = value.replaceAll("\\{", ":").replaceAll("}", "");
                var node = pathArgNodes.get(content);
                if (node != null) {
                    return node;
                }
                node = new TrieNode(content, this);
                pathArgNodes.put(content, node);
                return node;
            }

            if (value.equals("**")) {
                var content = "__";
                var node = pathArgNodes.get(content);
                if (node != null) {
                    return node;
                }
                node = new TrieNode(content, this);
                node.setLeaf();
                pathArgNodes.put(content, node);
                return node;
            }

            return children.computeIfAbsent(value, content -> new TrieNode(content, this));
        }

        public String getContent() {
            return content;
        }

        @Nullable
        public TrieNode match(int index, String[] values) {
            if (index == values.length && leaf) {
                return this;
            }

            if (index >= values.length) {
                return null;
            }

            int nextIdx = index + 1;
            String value = values[index];
            var node = children.get(value);
            if (node != null) {
                node = node.match(nextIdx, values);
                if (node != null) {
                    return node;
                }
            }

            for (var child : pathArgNodes.values()) {
                var result = child.match(nextIdx, values);
                if (result != null) {
                    return result;
                }
            }
            return null;
        }
    }
}
