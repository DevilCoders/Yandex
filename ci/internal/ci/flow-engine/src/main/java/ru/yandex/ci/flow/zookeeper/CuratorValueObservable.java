package ru.yandex.ci.flow.zookeeper;

import java.util.function.Consumer;

import com.google.common.base.Preconditions;
import org.apache.curator.framework.CuratorFramework;
import org.apache.curator.framework.recipes.shared.EphemeralSharedValue;
import org.apache.curator.framework.recipes.shared.SharedValueListener;
import org.apache.curator.framework.recipes.shared.SharedValueReader;
import org.apache.curator.framework.state.ConnectionState;

public class CuratorValueObservable implements SharedValueListener, AutoCloseable {
    private final EphemeralSharedValue sharedValue;
    private Consumer<byte[]> onChange;

    public CuratorValueObservable(EphemeralSharedValue sharedValue) {
        Preconditions.checkNotNull(sharedValue);

        this.sharedValue = sharedValue;
        this.sharedValue.getListenable().addListener(this);
    }

    public void observe(Consumer<byte[]> onChange) {
        this.onChange = onChange;
    }

    public void start() {
        try {
            sharedValue.start();
        } catch (Exception exception) {
            throw new RuntimeException("Failed to start node", exception);
        }
    }

    public void reset() {
        try {
            sharedValue.setValue(new byte[0]);
        } catch (Exception exception) {
            throw new RuntimeException("Failed to reset value", exception);
        }
    }

    @Override
    public void close() throws Exception {
        sharedValue.getListenable().removeListener(this);
        sharedValue.close();
    }

    @Override
    public void valueHasChanged(SharedValueReader sharedValue, byte[] newValue) {
        Consumer<byte[]> handler = this.onChange;
        if (handler == null || newValue.length == 0) {
            return;
        }

        handler.accept(newValue);
    }

    @Override
    public void stateChanged(CuratorFramework client, ConnectionState newState) {

    }
}
