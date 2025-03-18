package ru.yandex.ci.tms.zk;

import java.nio.file.Paths;
import java.time.Duration;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import lombok.extern.slf4j.Slf4j;
import org.apache.curator.framework.CuratorFramework;
import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.ZooKeeper;
import org.apache.zookeeper.data.Stat;

@Slf4j
public class ZkCleaner {
    private final CuratorFramework curator;

    public ZkCleaner(CuratorFramework curator) {
        this.curator = curator;
    }

    public void cleanDir(String zkDir, Duration maxAge) throws Exception {
        log.info("Cleaning zk dir {}, removing nodes older than {}", zkDir, maxAge.toString());

        AtomicInteger processedCounted = new AtomicInteger();
        AtomicInteger removedCounted = new AtomicInteger();

        cleanRecursively(curator.getZookeeperClient().getZooKeeper(), zkDir, maxAge, processedCounted, removedCounted);
        log.info("Processed all {} ZK nodes, removed {}", processedCounted.get(), removedCounted.get());
    }

    private boolean cleanRecursively(ZooKeeper zk, String nodePath, Duration maxAge,
                                     AtomicInteger processedCounted,
                                     AtomicInteger removedCounted) throws InterruptedException, KeeperException {

        log(processedCounted, removedCounted);
        processedCounted.incrementAndGet();

        Stat stat = zk.exists(nodePath, false);
        if (stat == null) {
            return false;
        }

        boolean childFree = true;
        if (stat.getNumChildren() > 0) {
            log.info("Processing {} children of node {}", stat.getNumChildren(), nodePath);
            List<String> children = zk.getChildren(nodePath, false);
            for (String child : children) {
                String childPath = Paths.get(nodePath, child).toString();
                childFree &= cleanRecursively(zk, childPath, maxAge, processedCounted, removedCounted);
            }
        }

        if (!childFree) {
            log.info("Node {} still has children, skipping", nodePath);
            return false;
        }

        long ageMinutes = TimeUnit.MILLISECONDS.toMinutes(System.currentTimeMillis() - stat.getMtime());
        if (ageMinutes < maxAge.toMinutes()) {
            return false;
        }

        log.info("Removing node: {} (age {} minutes) ", nodePath, ageMinutes);
        zk.delete(nodePath, stat.getVersion());
        removedCounted.incrementAndGet();
        return true;
    }

    private void log(AtomicInteger processedCounted, AtomicInteger removedCounted) {
        int processed = processedCounted.get();
        if (processed > 0 && processed % 1_000 == 0) {
            log.info("Processed {}, removed {}", processed, removedCounted.get());
        }
    }

}
