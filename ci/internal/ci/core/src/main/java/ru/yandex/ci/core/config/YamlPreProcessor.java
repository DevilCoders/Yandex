package ru.yandex.ci.core.config;

import java.io.StringReader;
import java.util.Map;

import javax.annotation.Nonnull;

import org.yaml.snakeyaml.DumperOptions;
import org.yaml.snakeyaml.LoaderOptions;
import org.yaml.snakeyaml.Yaml;
import org.yaml.snakeyaml.constructor.Constructor;
import org.yaml.snakeyaml.error.YAMLException;
import org.yaml.snakeyaml.nodes.MappingNode;
import org.yaml.snakeyaml.nodes.Node;
import org.yaml.snakeyaml.nodes.SequenceNode;
import org.yaml.snakeyaml.nodes.Tag;
import org.yaml.snakeyaml.representer.Representer;
import org.yaml.snakeyaml.resolver.Resolver;

public class YamlPreProcessor {

    private YamlPreProcessor() {
        //
    }

    public static String preprocess(@Nonnull String yaml) {
        if (yaml.isEmpty()) {
            return yaml;
        }
        var processor = createYamlProcessor();

        var object = processor.load(yaml);
        if (object == null) {
            return "";
        }

        // Well, this is bad - parse YAML again just to validate for recursive anchors
        validateAnchors(processor.compose(new StringReader(yaml)));

        // A clean resolved YAML to feed Jackson
        return processor.dump(object);
    }

    public static Yaml createYamlProcessor() {
        var loaderOptions = new LoaderOptions();
        loaderOptions.setMaxAliasesForCollections(1024); // Should be enough for any manual flow, see CI-2426

        // Additional YAML transformation using 'snakeyaml':
        // * anchors
        // * merge
        return new Yaml(
                new Constructor(),
                new NonReusableRepresenter(),
                new DumperOptions(),
                loaderOptions,
                YamlParsers.fixResolver(new Resolver())
        );
    }


    private static void validateAnchors(Node node) {
        if (node.isTwoStepsConstruction()) {
            throw new YAMLException("Detected recursive YAML Anchor: " + node.getAnchor());
        }
        if (node instanceof SequenceNode actualNode) {
            for (var item : actualNode.getValue()) {
                validateAnchors(item);
            }
        } else if (node instanceof MappingNode actualNode) {
            for (var value : actualNode.getValue()) {
                validateAnchors(value.getKeyNode());
                validateAnchors(value.getValueNode());
            }
        }
    }

    // Do not reuse objects in order to prevent rendering objects as anchors
    private static class NonReusableRepresenter extends Representer {

        @Override
        protected Node representSequence(Tag tag, Iterable<?> sequence, DumperOptions.FlowStyle flowStyle) {
            representedObjects.clear();
            return super.representSequence(tag, sequence, flowStyle);
        }

        @Override
        protected Node representMapping(Tag tag, Map<?, ?> mapping, DumperOptions.FlowStyle flowStyle) {
            representedObjects.clear();
            return super.representMapping(tag, mapping, flowStyle);
        }
    }


}
