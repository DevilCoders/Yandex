package yandex.cloud.dashboard.generator;

import org.junit.Test;
import yandex.cloud.dashboard.generator.Generator.ResolvedVarSpec;

import java.util.List;

import static org.junit.Assert.assertEquals;

/**
 * @author ssytnik
 */
public class GeneratorTest {

    @Test
    public void transformHosts() {
        assertEquals(List.of("api-adapter-myt1.prod.cloud.yandex.net"),
                ResolvedVarSpec.strip(true, List.of("api-adapter-myt1.prod.cloud.yandex.net")));

        assertEquals(List.of("api-adapter-myt1"),
                ResolvedVarSpec.strip(false, List.of("api-adapter-myt1.prod.cloud.yandex.net")));
    }

}