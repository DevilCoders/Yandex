package yandex.cloud.ti.abcd.adapter;

import java.util.List;

import lombok.Value;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

@Value
class ListAccountsResult {

    @NotNull List<AbcdAccount> accounts;
    @Nullable String nextPageToken;

}
