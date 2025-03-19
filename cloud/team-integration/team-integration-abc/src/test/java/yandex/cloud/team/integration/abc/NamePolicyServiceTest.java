package yandex.cloud.team.integration.abc;

import org.assertj.core.api.Assertions;
import org.junit.Before;
import org.junit.Test;

public class NamePolicyServiceTest {

    private NamePolicyService namePolicyService;

    @Before
    public void createNamePolicyService() {
        namePolicyService = new NamePolicyServiceImpl();
    }

    @Test
    public void allowed() {
        var normalizedName = namePolicyService.normalizeName("s3api");
        Assertions.assertThat(normalizedName)
                .isEqualTo("s3api");
    }

    @Test
    public void testNormalizeLabel() {
        var normalizedLabel = namePolicyService.normalizeLabel("5*event__PROMO__2019!");
        Assertions.assertThat(normalizedLabel)
                .isEqualTo("5-event__promo__2019-");
    }

    @Test
    public void testNormalizeLeadingDigit() {
        var normalizedName = namePolicyService.normalizeName("5*event__promo__2019!");
        Assertions.assertThat(normalizedName)
                .isEqualTo("five-event-promo-2019");
    }

    @Test
    public void testNormalizeNonPrintable() {
        var normalizedName = namePolicyService.normalizeName("__WIKI__");
        Assertions.assertThat(normalizedName)
                .isEqualTo("wiki");
    }

    @Test
    public void testNormalizeLongName() {
        var normalizedName = namePolicyService.normalizeName(
                "--1234567890:1234567890.1234567890+-1234567890*1234567890__1234567890__");
        Assertions.assertThat(normalizedName)
                .isEqualTo("one234567890-1234567890-1234567890-1234567890-1234567890-123456");
    }

    @Test
    public void testNormalizeLongLabel() {
        var normalizedLabel = namePolicyService.normalizeLabel(
                "--1234567890:1234567890.1234567890+-1234567890*1234567890__1234567890__");
        Assertions.assertThat(normalizedLabel)
                .isEqualTo("-1234567890-1234567890-1234567890-1234567890-1234567890__123456");
    }

    @Test
    public void testNormalizeNonPrintableName() {
        var normalizedName = namePolicyService.normalizeName("__");
        Assertions.assertThat(normalizedName)
                .isEmpty();
    }

}
