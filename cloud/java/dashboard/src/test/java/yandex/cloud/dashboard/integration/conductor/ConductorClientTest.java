package yandex.cloud.dashboard.integration.conductor;

import org.junit.Ignore;
import org.junit.Test;
import yandex.cloud.dashboard.integration.conductor.ConductorClient.Node;

import java.util.List;
import java.util.stream.Stream;

import static java.util.stream.Collectors.toList;
import static org.junit.Assert.assertEquals;

/**
 * @author ssytnik
 */
@Ignore("For manual run only")
public class ConductorClientTest {

    @Test
    public void getHosts() {
        assertEquals(adapterHosts("myt1", "myt2", "sas1", "sas2", "vla1", "vla2"),
                new ConductorClient().getHosts("cloud_prod_api-adapter"));
    }

    @Test
    public void getHostsShallow() {
        assertEquals(adapterHosts(), new ConductorClient().getHostsShallow("cloud_prod_api-adapter"));
        assertEquals(adapterHosts("vla1", "vla2"), new ConductorClient().getHostsShallow("cloud_prod_api-adapter_vla"));
    }

    @Test
    public void getSubGroups() {
        assertEquals(adapterGroups("myt", "sas", "vla"), new ConductorClient().getSubGroups("cloud_prod_api-adapter"));
        assertEquals(List.of(), new ConductorClient().getSubGroups("cloud_prod_api-adapter_vla"));
    }

    @Test
    public void getTree() {
        List<Node> tree = new ConductorClient().getTree("cloud_prod_api-adapter");
        System.out.println("tree:");
        tree.forEach(System.out::println);
        assertEquals(10, tree.size());
    }


    private List<String> adapterHosts(String... baseNames) {
        return Stream.of(baseNames).map(s -> "api-adapter-" + s + ".prod.cloud.yandex.net").collect(toList());
    }

    private List<String> adapterGroups(String... baseNames) {
        return Stream.of(baseNames).map(s -> "cloud_prod_api-adapter_" + s).collect(toList());
    }

}