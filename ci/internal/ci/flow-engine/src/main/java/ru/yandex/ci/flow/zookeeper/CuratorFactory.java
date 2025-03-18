package ru.yandex.ci.flow.zookeeper;

import org.apache.curator.framework.CuratorFramework;
import org.apache.curator.framework.recipes.shared.EphemeralSharedValue;

public class CuratorFactory {
    private final CuratorFramework curatorFramework;

    public CuratorFactory(CuratorFramework curatorFramework) {
        this.curatorFramework = curatorFramework;
    }

    public EphemeralSharedValue createSharedValue(String nodePath) {
        return new EphemeralSharedValue(curatorFramework, nodePath, new byte[0]);
    }

    public CuratorValueObservable createValueObservable(String nodePath) {
        var sharedValue = createSharedValue(nodePath);
        return new CuratorValueObservable(sharedValue);
    }
}
