package ru.yandex.ci.common.ydb;

import java.sql.SQLException;

import javax.annotation.Nonnull;

import com.yandex.ydb.jdbc.YdbConnection;
import lombok.AllArgsConstructor;
import org.springframework.jdbc.datasource.JdbcTransactionObjectSupport;
import org.springframework.transaction.PlatformTransactionManager;
import org.springframework.transaction.annotation.Isolation;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.transaction.support.DefaultTransactionStatus;
import org.springframework.transaction.support.TransactionTemplate;

@AllArgsConstructor
public class YdbExecutor {

    @Nonnull
    private final PlatformTransactionManager transactionManager;

    @Transactional(isolation = Isolation.SERIALIZABLE) // TRANSACTION_SERIALIZABLE_READ_WRITE
    public void executeRW(SqlExecute action) {
        executeImpl(action);
    }

    @Transactional(isolation = Isolation.REPEATABLE_READ) // ONLINE_CONSISTENT_READ_ONLY
    public void executeRO(SqlExecute action) {
        executeImpl(action);
    }

    private void executeImpl(SqlExecute action) {
        // Basically a helper but with full hierarchical transaction support, retries, etc.
        new TransactionTemplate(transactionManager).execute(status -> {
            var defaultStatus = (DefaultTransactionStatus) status;
            var txSupport = (JdbcTransactionObjectSupport) defaultStatus.getTransaction();
            var holder = txSupport.getConnectionHolder();
            try {
                action.execute((YdbConnection) holder.getConnection());
            } catch (SQLException sql) {
                throw new RuntimeException("Unable to execute action", sql);
            }
            return null;
        });
    }

    public interface SqlExecute {
        void execute(YdbConnection connection) throws SQLException;
    }
}
