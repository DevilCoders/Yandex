#include <library/cpp/coroutine/engine/impl.h>
#include <util/string/cast.h>

size_t X = 0;
size_t N = 0;

void Func(TCont* c, void*) {
    for (size_t i = 0; i < N; ++i) {
        ++X;
        c->Yield();
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        return 0;
    }

    N = FromString<size_t>(argv[1]);

    TContExecutor e(8192);

    for (size_t i = 0; i < 100; ++i) {
        e.Create(Func, nullptr, "y");
    }

    e.Execute();

    Cout << X << Endl;
}
