package yandex.cloud.ti.abcd.provider;

import java.util.List;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.config.ConfigLoader;

public class MappedQuotaMetricTest {

    @Test
    public void loadMappedQuotaMetric() throws Exception {
        MappedQuotaMetric mappedQuotaMetric = ConfigLoader.loadConfigFromStringContent(MappedQuotaMetric.class, """
                name: cloudQuotaMetric1
                abcdResource:
                    resourceKey:
                        resourceTypeKey: resourceTypeKey1
                        resourceSegmentKeys:
                            -   segmentationKey: resourceTypeKey1.segmentationKey1
                                segmentKey: resourceTypeKey1.segmentKey1
                            -   segmentationKey: resourceTypeKey1.segmentationKey2
                                segmentKey: resourceTypeKey1.segmentKey2
                    unitKey: resourceTypeKey1.unit
                """
        );
        Assertions.assertThat(mappedQuotaMetric)
                .isEqualTo(new MappedQuotaMetric(
                        "cloudQuotaMetric1",
                        new MappedAbcdResource(
                                new AbcdResourceKey(
                                        "resourceTypeKey1",
                                        List.of(
                                                new AbcdResourceSegmentKey(
                                                        "resourceTypeKey1.segmentationKey1",
                                                        "resourceTypeKey1.segmentKey1"
                                                ),
                                                new AbcdResourceSegmentKey(
                                                        "resourceTypeKey1.segmentationKey2",
                                                        "resourceTypeKey1.segmentKey2"
                                                )
                                        )
                                ),
                                "resourceTypeKey1.unit"
                        )
                ));
    }

    @Test
    public void loadMappedQuotaMetric_withEmptyResourceSegmentKeys() throws Exception {
        MappedQuotaMetric mappedQuotaMetric = ConfigLoader.loadConfigFromStringContent(MappedQuotaMetric.class, """
                name: cloudQuotaMetric1
                abcdResource:
                    resourceKey:
                        resourceTypeKey: resourceTypeKey1
                        resourceSegmentKeys:
                    unitKey: resourceTypeKey1.unit
                """
        );
        Assertions.assertThat(mappedQuotaMetric)
                .isEqualTo(new MappedQuotaMetric(
                        "cloudQuotaMetric1",
                        new MappedAbcdResource(
                                new AbcdResourceKey(
                                        "resourceTypeKey1",
                                        List.of()
                                ),
                                "resourceTypeKey1.unit"
                        )
                ));
    }

    @Test
    public void loadMappedQuotaMetric_withoutResourceSegmentKeys() throws Exception {
        MappedQuotaMetric mappedQuotaMetric = ConfigLoader.loadConfigFromStringContent(MappedQuotaMetric.class, """
                name: cloudQuotaMetric1
                abcdResource:
                    resourceKey:
                        resourceTypeKey: resourceTypeKey1
                    unitKey: resourceTypeKey1.unit
                """
        );
        Assertions.assertThat(mappedQuotaMetric)
                .isEqualTo(new MappedQuotaMetric(
                        "cloudQuotaMetric1",
                        new MappedAbcdResource(
                                new AbcdResourceKey(
                                        "resourceTypeKey1",
                                        List.of()
                                ),
                                "resourceTypeKey1.unit"
                        )
                ));
    }

}
