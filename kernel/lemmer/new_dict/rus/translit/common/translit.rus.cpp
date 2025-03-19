#include <kernel/lemmer/translate/dictfrombin.h>
#include <util/generic/singleton.h>

static const char* GetLine(size_t i) {
    static const char data[][16001] = {
        "eJzNlE9Mm2Ucx1+2BzZg48+ADTZgK7DBgEH70tLSdtBSFo0RdtCYmHiQbmUwu7UBCnHRg4kHvZgYPXgxxkSTJVuWJSbGgwfjZUs86MFEEy+evHnw4MGDid/f7/22+bnJAiFLhHze7+f58z7P87Z9f2kv+KsHTaANdIIe0AcGwAgYB2EwDVJgDiyA58Bl8AJ4CeTBDbAOtsEb4C3wNngHvAc+AB+Bj8Gn4HNwF9wHX4FvwEPwA/gZ/AJ+Bb+B38Ef4E/wF/gb1NV5XgNoBq2gA3SDfjAIhsEYmARREAdh57t5N+s8L+RFXMLF1ZIu5cIuQp+tZVot6mLMWZc0llWPuYuai5oLGMm4HO+cQsuHXcKe6VqmdCyB+Qu6V9RNI2X3aib1XHKmGdwfjGcwP6z3pnDux3tDOl92S+t9KbVYzRZrNqfXrD6L3J3BOlFd4SLmz2vv825JzxCYPKnsNYWcw9wFnT2H9WT9rPZe0nUzWC+NZ5CRjLZjej4fGflXe5HtxCPjiUfGc7qXZJbteRCpuZxPWs/iKeQEWVyDbzLLe3NYUXJJM+fkm5VnW6pZSHuDOZK+ewZz59USTr7/4Dfh627SG5y2ekrf+frJB+nr3Mu4Tpi3rBG0gHbQBbpBryczPW8IjHrBfB/EQRpkQI5v2ovgZfAKeBWsgOugBDb41r3Jt+5d8D74kG/cJ+AzcBvcAffAF+BL8DX4FjwA34HvwY/gJ7DsVbxN3SHvrXm3kFe8IvryaiVe5b2/6hX0WkJPNeVUBe+mnrWEylDRmdfgW8hVrHyjlkXYGuau6KrXcZU9XtM9ixjZwkrSW8E6N3GuovaXtF8qzzLmFNXKet3AvZ7OXMesq+oF7BTktp5NznQNWcb4imYR86/oimWcq6z1bBmr5TFS0Nq2jpTdt5F5bVd43WBu6udT0G9lFXuU1co6e0NnV7CCXOWzu6UmZ91CexUjm+pr+omW6AX9dqt7but/NcVe1xXr+GvTnDCNKgeYB5mOWc9sYB5iHuY9TWw3M48wjzJbmK2c3872MbY72e7i3t1s9zBPMk8xe3lfP9unmWeYIeYAc5DPMsz2CPM81xlje5x5gTnB8TDTZ0Y5HmNOM+Mcn2E7yUwx097jf3XGDxg/aNwZrzfeYPyQ8cPGG403GW82fsT4UeMtxluNtxlvN37MeIfxTuNdxo8bP2G823iP8ZPGTxnvNd5nvN/4aeNnjIeMDxgfND5k/Kzxc8aHjY8YP2981PiY8XHjF4xPGJ80HjYeMe4bnzIeNR4zPm08bjxhfMZ40niKWf1N77eOSDYy91JPJNuYtq5IdjBtfZE8zjzB3E29kexj7qXuSA4xzzLPMf+rHkmOMp9UlyQnmWFmhOkzp5hPqleSCeb/qW49jRq2m3pmfafatpPvp+Y9jfq3G99NjbS+13q5U+3cqY7up6bupb7+AxYcyZM=",
        ""
    };

    return data[i];
}

namespace NLemmer{
namespace NData {
    struct TTranslationRuslit: public TTranslationDictFromBin {
        TTranslationRuslit()
            : TTranslationDictFromBin(GetLine, 16001)
        {}
    };

    const TTranslationDict * GetTranslationRuslit() {
        const TTranslationRuslit* ret = Singleton<TTranslationRuslit>();
        return &(*ret)();
    }
}
}
