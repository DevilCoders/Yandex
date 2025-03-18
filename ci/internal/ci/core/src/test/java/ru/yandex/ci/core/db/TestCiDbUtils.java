package ru.yandex.ci.core.db;

import java.util.function.Supplier;

import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import yandex.cloud.repository.db.TxManager;
import ru.yandex.lang.NonNullApi;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.mock;

@NonNullApi
public final class TestCiDbUtils {

    private TestCiDbUtils() {
    }

    @SuppressWarnings({"unchecked", "rawtypes"})
    public static void mockToCallRealTxMethods(CiMainDb dbMock) {
        var call = new Answer() {
            @Override
            public Object answer(InvocationOnMock invocation) {
                return invocation.getArgument(0, Supplier.class).get();
            }
        };

        var lenient = lenient();
        lenient.doReturn(dbMock).when(dbMock).withName(anyString());

        lenient.doAnswer(call).when(dbMock).tx(any(Supplier.class));
        lenient.doAnswer(call).when(dbMock).currentOrTx(any(Supplier.class));
        lenient.doCallRealMethod().when(dbMock).tx(any(Runnable.class));
        lenient.doCallRealMethod().when(dbMock).currentOrTx(any(Runnable.class));

        var readonlyBuilder = mock(TxManager.ReadonlyBuilder.class);
        lenient.doReturn(readonlyBuilder).when(dbMock).readOnly();
        lenient.doAnswer(call).when(readonlyBuilder).run(any(Supplier.class));
        lenient.doCallRealMethod().when(readonlyBuilder).run(any(Runnable.class));

        lenient.doAnswer(call).when(dbMock).readOnly(any(Supplier.class));
        lenient.doAnswer(call).when(dbMock).currentOrReadOnly(any(Supplier.class));
        lenient.doCallRealMethod().when(dbMock).readOnly(any(Runnable.class));
        lenient.doCallRealMethod().when(dbMock).currentOrReadOnly(any(Runnable.class));

        var scanBuilder = mock(TxManager.ScanBuilder.class);
        lenient.doReturn(scanBuilder).when(dbMock).scan();
        lenient.doAnswer(call).when(scanBuilder).run(any(Runnable.class));
        lenient.doAnswer(call).when(scanBuilder).run(any(Supplier.class));

    }

}
