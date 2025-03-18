package ru.yandex.ci.flow.zookeeper;

import java.nio.ByteBuffer;
import java.util.function.UnaryOperator;

import com.google.common.base.Preconditions;
import io.reactivex.Observable;
import io.reactivex.disposables.Disposables;
import org.apache.curator.CuratorConnectionLossException;
import org.apache.curator.framework.CuratorFramework;
import org.apache.curator.framework.recipes.atomic.AtomicValue;
import org.apache.curator.framework.recipes.atomic.EphemeralDistributedAtomicLong;
import org.apache.curator.framework.recipes.cache.ChildData;
import org.apache.curator.framework.recipes.cache.NodeCache;
import org.apache.curator.framework.state.ConnectionState;
import org.apache.curator.framework.state.ConnectionStateListener;
import org.apache.curator.retry.RetryOneTime;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.ci.flow.exceptions.CasAttemptsExceededException;

public class EntityVersionService {
    private final Logger log = LogManager.getLogger(this.getClass());

    private final CuratorFramework curatorFramework;
    private final String basePath;

    private final int maxCasAttempts;
    private final int sleepBetweenRetryMillis;

    public EntityVersionService(CuratorFramework curatorFramework, String basePath,
                                int maxCasAttempts, int sleepBetweenRetryMillis) {

        this.curatorFramework = curatorFramework;
        this.sleepBetweenRetryMillis = sleepBetweenRetryMillis;

        Preconditions.checkArgument(basePath.startsWith("/"), "basePath must start with /");
        Preconditions.checkArgument(basePath.endsWith("/"), "basePath must end with /");
        this.basePath = basePath;

        Preconditions.checkArgument(maxCasAttempts > 0, "maxCasAttempts must be greater than 0");
        this.maxCasAttempts = maxCasAttempts;
    }

    public Observable<Long> observe(String entityId) {
        return Observable.create(subscriber -> {
            String counterPath = getCounterPath(entityId);
            NodeCache nodeCache = new NodeCache(curatorFramework, counterPath);

            nodeCache.getListenable().addListener(() -> {
                long value = getCounterValue(nodeCache);
                log.info("Received version update of entity {}, version = {}, path = {}", entityId, value, counterPath);
                subscriber.onNext(value);
            });

            log.info("Subscribed to {}", counterPath);

            ConnectionStateListener connectionStateListener = (client, newState) -> {
                if (newState == ConnectionState.LOST ||
                    newState == ConnectionState.SUSPENDED) {
                    subscriber.onError(new CuratorConnectionLossException());
                }
            };
            curatorFramework.getConnectionStateListenable().addListener(connectionStateListener);

            subscriber.setDisposable(Disposables.fromAction(() -> {
                log.info("Closing shared counter: {}", counterPath);
                curatorFramework.getConnectionStateListenable().removeListener(connectionStateListener);
                nodeCache.close();
            }));

            nodeCache.start();
        });
    }

    public void setIfLessThan(String entityId, long newValue) {
        compareAndSetWithRetries(
            entityId,
            "set value to " + newValue,
            oldValue -> newValue > oldValue ? newValue : null
        );
    }

    public void increment(String entityId) {
        compareAndSetWithRetries(
            entityId,
            "increment",
            value -> value + 1
        );
    }

    private void compareAndSetWithRetries(
        String entityId,
        String valueModifierDescription,
        UnaryOperator<Long> valueModifier
    ) {
        var path = getCounterPath(entityId);
        var revisionCounter = new EphemeralDistributedAtomicLong(
            curatorFramework, path, new RetryOneTime(sleepBetweenRetryMillis)
        );

        try {
            // compareAndSet не работает, если ноды не существует. Данный метод атомарно создаёт ноду
            // тогда и только тогда, когда ноды не существует.
            revisionCounter.initialize(0L);

            for (int attempt = 1; attempt <= maxCasAttempts; ++attempt) {
                AtomicValue<Long> oldAtomicValue = revisionCounter.get();
                long currentValue = oldAtomicValue.preValue();
                Long newValue = valueModifier.apply(currentValue);

                log.info(
                    "Trying to set {} version to {}, attempt {} (currentValue = {})",
                    entityId, newValue, attempt, currentValue
                );

                if (newValue == null) {
                    return;
                }

                AtomicValue<Long> newAtomicValue = revisionCounter.compareAndSet(currentValue, newValue);
                if (newAtomicValue.succeeded()) {
                    log.info("Version of {} has been set to {} successfully", entityId, newValue);
                    return;
                }
            }

        } catch (Exception e) {
            throw new RuntimeException(
                String.format(
                    "Exception while trying to %s revision counter, path = %s",
                    valueModifierDescription, path
                ),
                e
            );
        }

        throw new CasAttemptsExceededException(
            String.format(
                "Unable to %s version value in ZK in %d attempts, entityId = %s",
                valueModifierDescription, maxCasAttempts, entityId
            )
        );
    }

    private String getCounterPath(String entityId) {
        return basePath + entityId;
    }

    private long getCounterValue(NodeCache nodeCache) {
        ChildData currentData = nodeCache.getCurrentData();
        if (currentData == null) {
            return 0;
        }

        byte[] data = currentData.getData();
        if ((data == null) || (data.length == 0)) {
            return 0;
        }

        ByteBuffer wrapper = ByteBuffer.wrap(data);
        return wrapper.getLong();
    }

}
