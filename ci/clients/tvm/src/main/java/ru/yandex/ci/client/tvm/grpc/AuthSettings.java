package ru.yandex.ci.client.tvm.grpc;

import java.util.function.Predicate;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import io.grpc.MethodDescriptor;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.client.blackbox.BlackboxClient;
import ru.yandex.passport.tvmauth.TvmClient;

@Value
@Builder
public class AuthSettings {
    public static final Predicate<MethodDescriptor<?, ?>> REQUIRED = method -> true;
    public static final Predicate<MethodDescriptor<?, ?>> NOT_REQUIRED = method -> false;

    @Nonnull
    TvmClient tvmClient;
    @Nonnull
    BlackboxClient blackbox;
    @Nonnull
    Predicate<MethodDescriptor<?, ?>> ignoreAuthMethods;
    @Nonnull
    Predicate<MethodDescriptor<?, ?>> mandatoryServiceTicket;
    @Nonnull
    Predicate<MethodDescriptor<?, ?>> mandatoryUserTicket;
    @Nullable
    String oAuthScope;

    boolean debug;

    public static class Builder {
        {
            // All service tickets are optional by default
            mandatoryServiceTicket = NOT_REQUIRED;

            // All user tickets are required by default
            mandatoryUserTicket = REQUIRED;

            // No additional methods to skip auth
            ignoreAuthMethods = NOT_REQUIRED;
        }

        public Builder ignoreAuthMethodsList(MethodDescriptor<?, ?>... ignore) {
            var set = Stream.of(ignore)
                    .map(MethodDescriptor::getFullMethodName)
                    .collect(Collectors.toSet());
            this.ignoreAuthMethods = method ->
                    set.contains(method.getFullMethodName());
            return this;
        }
    }
}
