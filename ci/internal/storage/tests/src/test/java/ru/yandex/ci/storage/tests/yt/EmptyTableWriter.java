package ru.yandex.ci.storage.tests.yt;

import java.util.List;
import java.util.concurrent.CompletableFuture;

import org.apache.commons.lang3.NotImplementedException;

import ru.yandex.yt.rpcproxy.TRowsetDescriptor;
import ru.yandex.yt.ytclient.object.WireRowSerializer;
import ru.yandex.yt.ytclient.proxy.TableWriter;
import ru.yandex.yt.ytclient.tables.TableSchema;

@SuppressWarnings("rawtypes")
public class EmptyTableWriter implements TableWriter {
    @Override
    public WireRowSerializer getRowSerializer() {
        throw new NotImplementedException();
    }

    @Override
    public boolean write(List rows, TableSchema schema) {
        return true;
    }

    @Override
    public CompletableFuture<Void> readyEvent() {
        return CompletableFuture.completedFuture(null);
    }

    @Override
    public CompletableFuture<?> close() {
        return CompletableFuture.completedFuture(null);
    }

    @Override
    public TRowsetDescriptor getRowsetDescriptor() {
        throw new NotImplementedException();
    }

    @Override
    public CompletableFuture<TableSchema> getTableSchema() {
        throw new NotImplementedException();
    }

    @Override
    public void cancel() {

    }

    @Override
    public boolean write(List rows) {
        return true;
    }
}
