package ru.yandex.ci.engine.notification.xiva;

import javax.annotation.Nullable;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.test.TestData;

import static org.assertj.core.api.Assertions.assertThat;

class XivaNotifierDeferredImplTest extends CommonYdbTestBase {

    @Test
    void onLaunchStateChanged() {
        var launch = TestData.launchBuilder().build();
        var impl = new XivaNotifierImpl();
        Runnable runnable = () -> new XivaNotifierDeferredImpl(impl).onLaunchStateChanged(launch, launch);
        testDeferredCall(impl, runnable);
    }

    private void testDeferredCall(XivaNotifierImpl impl, Runnable runnable) {
        assertThat(impl.called).isFalse();
        runnable.run();
        assertThat(impl.called).isTrue();

        impl.called = false;
        db.currentOrTx(() -> {
            runnable.run();
            assertThat(impl.called).isFalse();
        });
        assertThat(impl.called).isTrue();
    }

    private static class XivaNotifierImpl implements XivaNotifier {

        private volatile boolean called = false;

        @Override
        public void onLaunchStateChanged(Launch updatedLaunch, @Nullable Launch oldLaunch) {
            called = true;
        }

        @Override
        public Common.XivaSubscription toXivaSubscription(XivaBaseEvent event) {
            called = true;
            return Common.XivaSubscription.getDefaultInstance();
        }

        @Override
        public void close() throws Exception {
            called = true;
        }
    }

}
