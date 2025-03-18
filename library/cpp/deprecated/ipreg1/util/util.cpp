#include <library/cpp/deprecated/ipreg1/lookup.h>
#include <iostream>

int main(int /* argc */, char** argv)
{
    const NIpreg::TLookup lookup(argv[1]);
    NIpreg::Net net = lookup.GetNet(argv[2]);
    std::cout << argv[2] << " => " << (net.IsYandex ? "yandex" : "") << (net.IsUser ? "/user" : "") << "\n";
}
