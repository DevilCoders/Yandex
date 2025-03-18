package ru.yandex.ci.storage.api.search;

import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.EnumSource;

import ru.yandex.ci.storage.core.Common.CheckStatus;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

public class LargeTestsSearchUnitTest {

    @ParameterizedTest
    @EnumSource(CheckStatus.class)
    void testToLargeTestStatusCheckedAll(CheckStatus status) {
        if (status == CheckStatus.UNRECOGNIZED) {
            assertThatThrownBy(() -> LargeTestsSearch.toLargeTestStatus(status))
                    .hasMessage("Internal error. Unable to map task status: UNRECOGNIZED");
        } else {
            assertThat(LargeTestsSearch.toLargeTestStatus(status))
                    .isNotNull();
        }
    }

}
