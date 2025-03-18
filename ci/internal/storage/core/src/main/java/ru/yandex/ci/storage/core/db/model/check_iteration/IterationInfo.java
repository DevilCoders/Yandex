package ru.yandex.ci.storage.core.db.model.check_iteration;

import java.util.List;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;
import org.apache.logging.log4j.util.Strings;

import ru.yandex.ci.core.flow.CiActionReference;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder(toBuilder = true)
public class IterationInfo {
    public static final IterationInfo EMPTY = IterationInfo.builder()
            .fastTargets(Set.of())
            .disabledToolchains(Set.of())
            .notRecheckReason("")
            .pessimized(false)
            .pessimizationInfo("")
            .advisedPool("")
            .fatalErrorInfo(null)
            .testenvCheckId("")
            .progress(0)
            .strongModePolicy(StrongModePolicy.BY_A_YAML)
            .autodetectedFastCircuit(false)
            .ciActionReferences(List.of())
            .build();

    Set<String> fastTargets;
    Set<String> disabledToolchains;

    String notRecheckReason;

    boolean pessimized;
    String pessimizationInfo;

    String advisedPool;

    @Deprecated
    @Nullable
    String fatalErrorMessage;
    @Deprecated
    @Nullable
    String fatalErrorDetails;

    @Nullable
    FatalErrorInfo fatalErrorInfo;

    String testenvCheckId;

    int progress;

    @Nullable // nullable for old values
    StrongModePolicy strongModePolicy;

    boolean autodetectedFastCircuit;

    @Nullable
    List<CiActionReference> ciActionReferences;

    public List<CiActionReference> getCiActionReferences() {
        return ciActionReferences == null ? List.of() : ciActionReferences;
    }

    public StrongModePolicy getStrongModePolicy() {
        return Objects.requireNonNullElse(strongModePolicy, StrongModePolicy.BY_A_YAML);
    }

    public IterationInfo pessimize(String info) {
        return this.toBuilder()
                .pessimized(true)
                .pessimizationInfo(info)
                .build();
    }

    public FatalErrorInfo getFatalErrorInfo() {
        if (fatalErrorInfo != null) {
            return fatalErrorInfo;
        }

        if (Strings.isNotEmpty(this.fatalErrorMessage)) {
            return FatalErrorInfo.builder()
                    .message(this.fatalErrorMessage)
                    .details(this.fatalErrorDetails)
                    .sandboxTaskId("")
                    .build();
        }

        return FatalErrorInfo.EMPTY;
    }
}
