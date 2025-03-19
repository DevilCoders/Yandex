package yandex.cloud.dashboard.integration.conductor;

import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableList;
import lombok.SneakyThrows;
import lombok.Value;
import org.apache.http.client.methods.HttpGet;
import yandex.cloud.dashboard.integration.common.AbstractHttpClient;
import yandex.cloud.dashboard.integration.common.DefaultResponseHandler;
import yandex.cloud.dashboard.util.Yaml;

import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.Function;

import static java.util.stream.Collectors.toList;
import static yandex.cloud.dashboard.util.ObjectUtils.sorted;

/**
 * Caching conductor client
 *
 * @author ssytnik
 */
public class ConductorClient extends AbstractHttpClient {
    private static final String DEFAULT_HOST = "c.yandex-team.ru";
    private static final Splitter splitter = Splitter.on('\n').omitEmptyStrings();
    private static final ConcurrentHashMap<String, Object> cache = new ConcurrentHashMap<>();

    public ConductorClient() {
        this(DEFAULT_HOST);
    }

    public ConductorClient(String host) {
        super(host);
    }

    public List<String> getHosts(String group) {
        return cached("/api/groups2hosts/" + group, r -> sorted(splitter.splitToList(r)));
    }

    public List<String> getHostsShallow(String group) {
        return cached("/api/groups2hosts/" + group + "?recursive=no", r -> sorted(splitter.splitToList(r)));
    }

    @SuppressWarnings("unchecked")
    public List<String> getSubGroups(String group) {
        return cached("/api/groups/" + group + "?format=yaml",
                r -> sorted((List<String>) ((Map<String, ?>) Yaml.parse(List.class, r, false).get(0)).get(":children")));
    }

    // FIXME this is wrong implementation for case when group contains both subgroups and hosts
    public List<Node> getTree(String group) {
        List<String> subGroups = getSubGroups(group);
        List<String> hosts = getHosts(group);

        List<Node> subList;
        if (subGroups.isEmpty()) {
            subList = hosts.stream()
                    .map(h -> new Node(h, List.of(h)))
                    .collect(toList());
        } else {
            subList = subGroups.stream()
                    .flatMap(sg -> getTree(sg).stream())
                    .collect(toList());
        }

        return ImmutableList.<Node>builder()
                .add(new Node(group, hosts))
                .addAll(subList)
                .build();
    }

    @Value
    public static class Node {
        String name;
        List<String> hosts;
    }


    @SuppressWarnings("unchecked")
    private <T> T cached(String pathAndQuery, Function<String, T> parser) {
        return (T) cache.computeIfAbsent(pathAndQuery, __ -> get(pathAndQuery, parser));
    }

    @SneakyThrows
    private <T> Object get(String pathAndQuery, Function<String, T> parser) {
//        System.out.println("Calling " + pathAndQuery);
        HttpGet httpGet = new HttpGet("https://" + host + pathAndQuery);
        String response = httpClient.execute(httpGet, new DefaultResponseHandler());
        return parser.apply(response);
    }
}
