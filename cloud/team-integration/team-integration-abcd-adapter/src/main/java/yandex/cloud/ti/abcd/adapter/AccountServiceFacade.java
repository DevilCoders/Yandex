package yandex.cloud.ti.abcd.adapter;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.ti.abc.repo.ListPage;

public interface AccountServiceFacade {

    @NotNull AbcdAccount createAccount(
            @NotNull CreateAccountParameters parameters
    );

    @NotNull AbcdAccount getAccount(
            @NotNull GetAccountParameters parameters
    );

    @NotNull ListPage<AbcdAccount> listAccounts(
            @NotNull ListAccountsParameters parameters
    );

    @NotNull AbcdAccount updateProvision(
            @NotNull UpdateProvisionParameters parameters
    );

}
