package ru.yandex.ci.engine.utils.bazinga;

import java.util.List;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.engine.utils.GraphTraverser;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

class GraphTraverserTest {

    @Test
    public void single() throws AYamlValidationException {
        assertThat(GraphTraverser.traverse(node("SINGLE")))
                .containsExactly("single");
    }

    @Test
    public void twoChain() throws AYamlValidationException {
        assertThat(GraphTraverser.traverse(node("B"), node("A", "B")))
                .containsExactly("b", "a <- b");
    }

    @Test
    public void twoIndependent() throws AYamlValidationException {
        assertThat(GraphTraverser.traverse(node("B"), node("A")))
                .containsExactly("b", "a");
    }

    @Test
    public void chainRightOrder() throws AYamlValidationException {
        assertThat(GraphTraverser.traverse(
                node("C"),
                node("B", "C"),
                node("A", "B")
        ))
                .containsExactly(
                        "c",
                        "b <- c",
                        "a <- b <- c"
                );
    }

    @Test
    public void chainReverseOrder() throws AYamlValidationException {
        assertThat(GraphTraverser.traverse(
                node("A", "B"),
                node("B", "C"),
                node("C")
        ))
                .containsExactly(
                        "a <- b <- c",
                        "b <- c",
                        "c"
                );
    }

    @Test
    public void chainNoOrder() throws AYamlValidationException {
        assertThat(GraphTraverser.traverse(
                node("B", "C"),
                node("A", "B"),
                node("C")
        ))
                .containsExactly(
                        "b <- c",
                        "a <- b <- c",
                        "c"
                );
    }

    @Test
    public void multipleParent() throws AYamlValidationException {
        assertThat(GraphTraverser.traverse(
                node("A"),
                node("B"),
                node("C", "A", "B")
        ))
                .containsExactly(
                        "a",
                        "b",
                        "c <- (a, b)"
                );
    }

    @Test
    public void diamond() throws AYamlValidationException {
        List<NodeImpl> nodes = List.of(
                node("A"),
                node("B", "A"),
                node("C", "A"),
                node("D", "B", "C")
        );

        assertThat(GraphTraverser.traverse(nodes))
                .containsExactly(
                        "a",
                        "b <- a",
                        "c <- a",
                        "d <- (b <- a, c <- a)"
                );

        assertThat(nodes).extracting(NodeImpl::getCreateCallCount).containsOnly(1);
    }

    @Test
    public void smallCycle() {
        assertThatThrownBy(() -> GraphTraverser.traverse(node("A", "B"), node("B", "A")))
                .hasMessageContaining("cycle");
    }

    @Test
    public void bigCycle() {
        assertThatThrownBy(() -> GraphTraverser.traverse(node("A", "B"), node("B", "C"), node("C", "A")))
                .hasMessageContaining("cycle");
    }

    @Test
    public void selfCycle() {
        assertThatThrownBy(() -> GraphTraverser.traverse(node("A", "A")))
                .hasMessageContaining("cycle");
    }

    private static NodeImpl node(String key, String... parents) {
        return new NodeImpl(key, parents);
    }

    private static class NodeImpl implements GraphTraverser.Node<String> {
        private int createCallCount = 0;
        private final String key;
        private final List<String> parents;

        private NodeImpl(String key, String... parents) {
            this.key = key;
            this.parents = List.of(parents);
        }

        @Override
        public String key() {
            return key;
        }

        @Override
        public List<String> parents() {
            return parents;
        }

        @Override
        public String create(List<String> parents) {
            createCallCount++;
            String that = this.key.toLowerCase();

            if (parents.isEmpty()) {
                return that;
            }
            if (parents.size() == 1) {
                return that + " <- " + parents.get(0);
            }

            return that + " <- (" + String.join(", ", parents) + ")";
        }

        public int getCreateCallCount() {
            return createCallCount;
        }
    }
}
