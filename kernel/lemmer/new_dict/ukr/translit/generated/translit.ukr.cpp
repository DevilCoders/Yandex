#include <kernel/lemmer/translate/dictfrombin.h>
#include <util/generic/singleton.h>

static const char* GetLine(size_t i) {
    static const char data[][16001] = {
        "eJzNlE9oHGUYxr9NvzRNmqZJm7RJm6TdNG3TNE13J7v5t5tkN5uiiE0PiiDxYNZsm9Ztd8lms1j0IHjQiyB68CIieBAUEQQRvNhTBQ96EPTgxZM3Dx48eBCf951nl7fUlIRQMOE3z++b+f7NzM6bduFfM2gDnaAb9IEBMARGwBiIgUmQAgtgCTwFroFnwHMgD26DDVADr4LXwRvgTfA2eBe8Dz4AH4FPwGfgC/A1+BZ8B34Ev4BfwW/gd/AH+BP8Bf4G/4CmiHMtoB10gm7QBwbBMBgBYyAGkmBa3Ad+0c9756Iu7lciU2qzPuVjPk6fb2RaLeErzHk/ayyrXvFzmknNJVzJ+BxHTqAVwK5gzXQjU3ptJTLnl3SthJ9Eyur1nPWyL9nTDMaH1zOYP6ZjU9j3w2ej2l9WS+u4lFqlYcmGLegxq/ciozOYJ6EzzKH/op592i/rHkKTO5W1JrTXAuaR2eTsAkYuaV7ReTOYL417SGGmjLaTur8AGX+gfZXtlciD1+vtq3wf8jTrmdUeOewxx97iskNpPYn7kD1kcQzHZjk2hzUllzVzXt6t3N1yw6J6NuwTZqDzOuQTGLWothJJYJ/z6hVdo77vwAf67MMMdIZrOI6b76wVdIAu0AN6Qb+Tnvi9glEX9g/AFEiDDMjxW3sWPA9eAC+C6+AWKIEKeAW8xu/uLfAOeI/f3IfgY35zn4LPwZfgK/ANuAfug+/BD+An8DNYdVW3qSvk3U13F/mSK+JcXq3Eo3z5a66gxxLO1FN2VXB3dK8l1Iaq9rwB30KuY+bbjSzCbqLvdZ31Fo6yxsu6ZhFXtjCTnK1injvYV1HPl/S81J5V9CmqlfVYwVinPTfQa029gJXCrOneZE83kGWMXMV+C+pr+mTL2FdZK9oqZsujR0Gr2wZSVq8h89qu8lhhburzKehbWccaZbWy9q5o7ypmkKM8u7tqstct3EFBq+cWrqyjzybPyrMtaWUN16zpfz1r+u5lxgh/bZrjplGnibmP6ZnNzP3MFuYBjmlj+yCznXmI2cE8zP5dbB9hu5vtHq7dy3Yf8wTzJLOf4wbZPsU8zYwyh5hneC/n2R5hXuA8F9keY15ijvN6jBkwE7yeZE4yp3h9hu1ZZoqZdg//RYw3Gd9n3BtvNr7feIvxA8ZbjbcZP2i83fgh4x3GDxvvNN5l/Ijxo8a7jfcYP2b8uPFe433GTxg/abzf+IDxQeOnjJ82HjU+ZPyM8WHjZ42fM37e+IjxC8ZHjV80Pmb8kvFx45eNx4zHjQfGJ4wnjCeNTxqfMj5tfMb4rPEUs/6b3msdkWxl7qaeSHYybV2RPMq09UXyGPM4cyf1RnKAuZu6IznMPMs8x/yveiQ5ynxUXZK8zIwx48yAOcF8VL2SnGb+n+rW46hhO6ln1rerbdv5Xmre46h/O/Gd1Ejru62X29XO7eroXmrqburrvy50y7c=",
        ""
    };

    return data[i];
}

namespace NLemmer {
namespace NData {
    struct TTranslationUkrlit: public TTranslationDictFromBin {
        TTranslationUkrlit()
            : TTranslationDictFromBin(GetLine, 16001)
        {}
    };

    const TTranslationDict * GetTranslationUkrlit() {
        const TTranslationUkrlit* ret = Singleton<TTranslationUkrlit>();
        return &(*ret)();
    }
}
}
