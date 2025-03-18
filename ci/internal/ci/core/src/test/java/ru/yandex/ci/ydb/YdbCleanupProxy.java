package ru.yandex.ci.ydb;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import com.google.common.collect.Lists;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.ArrayUtils;

import yandex.cloud.repository.db.Table;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.TransactionSupportDefault;

/**
 * Что тут происходит: все таблицы, которые мы затронули в рамках тестового запуска,
 * будут автоматически очищены перед началом каждого теста. Старое решение с очисткой таблицы в отдельной транзакции
 * больше не работает.
 * <p>
 * Как это работает: обращения во все методы получения таблиц (cronJobs(), launches(), ... все остальные)
 * проходят через этот класс, т.к. все ...Db...Impl классы обернуты через
 * {@link #withCleanupProxy(TransactionSupportDefault, ExecutorService)}
 * <p>
 * Специальный тестовый {@link ru.yandex.ci.test.YdbCleanupTestListener} дергает метод {@link #reset()} перед каждым
 * JUnit тестом, выполняя необходимую очистку.
 *
 * @param <T> маркер классов, которые можно заворачивать в этот прокси
 */
@Slf4j
public class YdbCleanupProxy<T extends TransactionSupportDefault> implements InvocationHandler, YdbCleanupReset {

    private static final Map<Class<?>, Set<Method>> MODIFIED_TABLES = new ConcurrentHashMap<>();

    private final T impl;

    private final ExecutorService executor;
    private final Map<Class<?>, Method> allTables;
    private final Set<Method> modifiedTables; // Reused across multiple proxies
    private final AtomicBoolean firstRun = new AtomicBoolean(true);

    public YdbCleanupProxy(T impl, ExecutorService executor) {
        log.info("[{}] Registering cleanup for {}", this, impl.getClass());
        this.executor = executor;

        this.modifiedTables = MODIFIED_TABLES.computeIfAbsent(impl.getClass(), type -> ConcurrentHashMap.newKeySet());

        this.impl = impl;
        this.allTables = Stream.of(impl.getClass().getDeclaredMethods())
                .filter(method -> method.getParameterCount() == 0)
                .filter(method ->
                        KikimrTableCi.class.isAssignableFrom(method.getReturnType()) ||
                                Table.class.isAssignableFrom(method.getReturnType()))
                .peek(method -> log.info("Register automatic cleanup for [{}]", method.getReturnType()))
                .collect(Collectors.toMap(Method::getReturnType, Function.identity()));
    }

    @Override
    public void reset() {
        if (firstRun.compareAndSet(true, false)) {
            resetTables(allTables.values());
        } else {
            resetTables(modifiedTables);
            modifiedTables.clear(); // till next time
        }
    }

    @Nullable
    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        if ("reset".equals(method.getName())) {
            this.reset();
            return null; // ---
        }

        var type = method.getReturnType();
        var acceptMethod = allTables.get(type);
        if (acceptMethod != null) {
            if (modifiedTables.add(acceptMethod)) {
                log.info("[{}] Register table for cleanup: {}", this, type);
            }
        }
        try {
            return method.invoke(impl, args);
        } catch (InvocationTargetException e) {
            throw e.getCause(); // rethrow original exception
        }
    }

    private void resetTables(Collection<Method> methods) {
        var now = System.currentTimeMillis();
        var futures = Lists.partition(List.copyOf(methods), 3).stream() // Fit into 16 threads Executor
                .map(this::deleteAll)
                .toList();

        Exception firstError = null;
        for (var future : futures) {
            try {
                future.get();
            } catch (Exception e) {
                log.error("Error when cleaning table", e);
                if (firstError == null) {
                    firstError = e;
                }
            }
        }

        log.info("Cleaning {} tables complete in {} msec", methods.size(), System.currentTimeMillis() - now);

        if (firstError != null) {
            throw new RuntimeException(firstError);
        }
    }

    private Future<?> deleteAll(List<Method> methods) {
        return executor.submit(() -> {
            impl.tx(() -> {
                for (var method : methods) {
                    var type = method.getReturnType();
                    log.info("[{}] Cleanup dirty table: {}", this, type);
                    try {
                        var actualTable = method.invoke(impl);
                        if (actualTable instanceof KikimrTableCi<?> kikimrTableCi) {
                            kikimrTableCi.deleteAll();
                        } else if (actualTable instanceof Table<?> table) {
                            table.deleteAll();
                        } else {
                            throw new IllegalStateException("Unexpected table: " + actualTable);
                        }
                    } catch (Exception e) {
                        throw new RuntimeException("Unable to cleanup dirty table " + type, e);
                    }
                }
            });
        });
    }


    @SuppressWarnings("unchecked")
    public static <T extends TransactionSupportDefault> T withCleanupProxy(T impl, ExecutorService executor) {
        var type = impl.getClass();
        var interfaces = ArrayUtils.add(type.getInterfaces(), YdbCleanupReset.class);
        var proxy = new YdbCleanupProxy<>(impl, executor);
        return (T) Proxy.newProxyInstance(type.getClassLoader(), interfaces, proxy);
    }

}
