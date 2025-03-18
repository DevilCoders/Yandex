#include <library/cpp/ssh/sign_key.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(Sign) {
    static const TString PrivateKeyName = ArcadiaSourceRoot() + "/library/cpp/ssh/ut/testdata/rsa_key";

    Y_UNIT_TEST(Sshkey) {
        TSSHPrivateKey priv(PrivateKeyName);

        const TString sign1 = Base64Decode(
            "fIHIitqCNc4mKvqbhax25DsIHE1YXmRH5plzKBsiKqJt7P4sNLzNhYssLjHwg25p+n6cQz/Hx+v1mYBlmKf/jO7ObWTW2UF4t0LGn411MmHx/NAvRthNet7yoBkkxUbulBDAHe0S6kbmCPdQoCSowwr/1Ft6sJp96dX8gAaYfx21gxneRQ5pXeDcALwy3B3/SObwv5+4S7Iu27PYQMUkOhOJkW6rQy1yZwZygs+9+zb94Iafbw9KK926S44IlUJByCKmYRXDhS4czSC8f4V7bJsfRjVW0MBnmxYvSR9Zd1qhTQgaaX10BZ6NZsxpWiZwHAcBbgAdK5guBSNdsc+3yw==");
        const TString data1 = "very_random_string";
        const TString sign2 = Base64Decode(
            "F+dNBfQe5sKLWQLWI/GXK4ZzI46iXUD+FNHugq3+Gha5Wpc8Kr8seIOK6Kuq35IUK0XY/mHB2ObTXAjPb/W7cACT967ja41ZUYJluIxjU9Y2arHtigHz356OPBF9QNGJENHMmfuSWoiGVRs/RNt7Fs+3marT45gWOvJYRJ3HptMoAy5Cxdzr37+ImytcPKRbl+DYoPoFDFVKpeq0RLz5L5gZ0cNphbmMOn0j58HMu5kpOM4h5PU79iWnq/dCzlNj1YhkupT2/I8WtegP5xuXYLa38Fh/rXYlsSGTBv7NCbZbtpwhh16k8FE/auAWESoe2aCPE388ZAUTrdCfSGfy5A==");
        const TString data2 = "very_poor_string";

        UNIT_ASSERT_EQUAL(sign1, priv.Sign(data1));
        UNIT_ASSERT_EQUAL(sign2, priv.Sign(data2));
    }

    Y_UNIT_TEST(SshkeyAgent) {
        TSSHPrivateKey priv(PrivateKeyName);

        const TString sign01 = Base64Decode(
            "AAAAB3NzaC1yc2EAAAEAfIHIitqCNc4mKvqbhax25DsIHE1YXmRH5plzKBsiKqJt7P4sNLzNhYssLjHwg25p+n6cQz/Hx+v1mYBlmKf/jO7ObWTW2UF4t0LGn411MmHx/NAvRthNet7yoBkkxUbulBDAHe0S6kbmCPdQoCSowwr/1Ft6sJp96dX8gAaYfx21gxneRQ5pXeDcALwy3B3/SObwv5+4S7Iu27PYQMUkOhOJkW6rQy1yZwZygs+9+zb94Iafbw9KK926S44IlUJByCKmYRXDhS4czSC8f4V7bJsfRjVW0MBnmxYvSR9Zd1qhTQgaaX10BZ6NZsxpWiZwHAcBbgAdK5guBSNdsc+3yw==");
        const TString data1 = "very_random_string";
        const TString sign02 = Base64Decode(
            "AAAAB3NzaC1yc2EAAAEAF+dNBfQe5sKLWQLWI/GXK4ZzI46iXUD+FNHugq3+Gha5Wpc8Kr8seIOK6Kuq35IUK0XY/mHB2ObTXAjPb/W7cACT967ja41ZUYJluIxjU9Y2arHtigHz356OPBF9QNGJENHMmfuSWoiGVRs/RNt7Fs+3marT45gWOvJYRJ3HptMoAy5Cxdzr37+ImytcPKRbl+DYoPoFDFVKpeq0RLz5L5gZ0cNphbmMOn0j58HMu5kpOM4h5PU79iWnq/dCzlNj1YhkupT2/I8WtegP5xuXYLa38Fh/rXYlsSGTBv7NCbZbtpwhh16k8FE/auAWESoe2aCPE388ZAUTrdCfSGfy5A==");
        const TString data2 = "very_poor_string";

        UNIT_ASSERT_EQUAL(sign01, priv.Sign(data1, TSSHPrivateKey::EPss::Skip, TSSHPrivateKey::EAgent::Simulate));
        UNIT_ASSERT_EQUAL(sign02, priv.Sign(data2, TSSHPrivateKey::EPss::Skip, TSSHPrivateKey::EAgent::Simulate));
    }

    Y_UNIT_TEST(SshkeyPss) {
        TSSHPrivateKey priv(PrivateKeyName);

        const TString sign1 = Base64Decode(
            "FqT+oNU77e01cfHGNCLESR6Uq1ywHEbIQc0IbCVH7MUEA/64XVQ011FCbhbCpHFtMLQD3hWHb92Ig2PONh+5l4SmJjRQb68GWm6ttcQlzT/5clKX86nSreJmRW14Vgnxa8VW8ttG0AEanwwDqDOO756xBxMd7xFiHhm+oKC+X6/LXeYbGc5OGURZL2A5hNF8MUS8Ps3ThhAwE9ak5Ul34y3EehpUBkigwxKb3n3EIW/RuQ4nqHTTy8vlRcKSAQ5xL2DjBHOkPZwZIiDCBHvD2Frg4005WGMijdrhG6EBuF9/ETRn7VzqyskGi5sfKaNloTq4cKCX6LHGbLOfz/EaUg==");
        const TString data1 = "very_random_string";
        const TString sign2 = Base64Decode(
            "Fje6T5og9+rEV6myjCL3lMFItfwiCYieyDaDHbBrW3tu2e/63DHOrIwADySMECz9GO5HU1JKu0yinhtWBSRlZi9X9WXi4DkLY30zMt/kN1gc6vHKmtZYtWpH4Ixha5KnA5gYyF+ItPZrXuzmTZv1p7caeOlrTIeR2NaCkI9n5ObT7IXogvZNyygIs9Gt2eFis4QghUCKxisKYWDgDuVoDrQdPc1jRFoZNn+lEKS6WRcpUPJAc8v0Z7doLqYhrXeD84pBP/5k/vYFC9R8EqeBq6L30TRR+wpyr9wkzK8FsVUsDz4BMdSAm4oXgAUALBXmWYaWXXWpzlME0lH14VxbHQ==");
        const TString data2 = "very_poor_string";

        const TString sign3 = priv.Sign(data1, TSSHPrivateKey::EPss::Use);
        const TString sign4 = priv.Sign(data2, TSSHPrivateKey::EPss::Use);
    }
}
