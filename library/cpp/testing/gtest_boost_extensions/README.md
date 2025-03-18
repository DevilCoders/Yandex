# Gtest extensions for boost types

Pretty printers to better support of boost types in gtest and gmock.

There is no native support for `boost::optional<T>` in gtest and 
it breaks compilation on using in tests  `boost::optional<T>` when T does not implement
`operator<<`, because gtest detects that `T=boost::optional<X>` has 
'operator<<' and tries to use it for printing.

So if you want to use `boost::optional<T>` in your tests - include 
header `<library/cpp/testing/gtest_boost_extensions/extensions.h>`
