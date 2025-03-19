package yandex.cloud.dashboard.util;

import org.junit.Test;

import static org.junit.Assert.assertEquals;

/**
 * @author ssytnik
 */
public class YamlTest {

    @Test
    @SuppressWarnings("ConstantConditions")
    public void testInclude() {
        String actual = Yaml.loadFile(
                getClass().getClassLoader().getResource("dashboard/testInclude/include-root.yaml").getFile());

        String expected = "" +
                "# comment1\n" +
                "# comment2\n" +
                "\n" +
                "version: 123\n" +
                "entities:\n" +
                "  # this is an entity for project 'projectA'\n" +
                "  type: entity\n" +
                "  project: projectA\n" +
                "  name: entityA\n" +
                "  queries:\n" +
                "    q1: query1\n" +
                "    q2: query2\n" +
                "  # this is an entity for project 'project B'\n" +
                "  type: entity\n" +
                "  project: project B\n" +
                "  name: entity B\n" +
                "  queries:\n" +
                "    q1: query1\n" +
                "    q2: query2\n";

        assertEquals(expected, actual);
    }

}