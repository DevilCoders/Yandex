package ru.yandex.ci.flow.engine.runtime.test_data;

import javax.annotation.Nullable;

import ru.yandex.ci.core.test.resources.ExpectedResources;
import ru.yandex.ci.core.test.resources.Flow451Result;
import ru.yandex.ci.core.test.resources.IntegerResource;
import ru.yandex.ci.core.test.resources.Res0;
import ru.yandex.ci.core.test.resources.Res1;
import ru.yandex.ci.core.test.resources.Res2;
import ru.yandex.ci.core.test.resources.Res3;
import ru.yandex.ci.core.test.resources.Res4;
import ru.yandex.ci.core.test.resources.Res5;
import ru.yandex.ci.core.test.resources.Resource451;
import ru.yandex.ci.core.test.resources.StringResource;

public class Resources {

    private Resources() {
        //
    }

    public static Res0 res0(String s) {
        return Res0.newBuilder().setS(s).build();
    }

    public static Res1 res1(String s) {
        return Res1.newBuilder().setS(s).build();
    }

    public static Res2 res2(String s) {
        return Res2.newBuilder().setS(s).build();
    }

    public static Res3 res3(String s) {
        return Res3.newBuilder().setS(s).build();
    }

    public static Res4 res4(String s) {
        return Res4.newBuilder().setS(s).build();
    }

    public static Res5 res5(String s) {
        return Res5.newBuilder().setS(s).build();
    }

    public static ExpectedResources expectedResources(@Nullable String res1, @Nullable String res2) {
        var builder = ExpectedResources.newBuilder();
        if (res1 != null) {
            builder.getRes1Builder().setS(res1);
        }
        if (res2 != null) {
            builder.getRes2Builder().setS(res2);
        }
        return builder.build();
    }

    public static Resource451 resource451(int value) {
        return Resource451.newBuilder().setValue(value).build();
    }

    public static StringResource stringResource(String value) {
        return StringResource.newBuilder().setString(value).build();
    }

    public static IntegerResource integerResource(int value) {
        return IntegerResource.newBuilder().setValue(value).build();
    }

    public static Flow451Result flow451Result(int value) {
        return Flow451Result.newBuilder().setValue(value).build();
    }
}
