# rseq

This library provides initialization code for librseq. We can't use librseq directly because of
licence issues.

This library is EXPERIMENTAL.

Code was tested with 4.19 search kernel.

`__rseq_abi` registration should interoperate with glibc, once glibc supports
rseq registration.

Current implementation is unsafe to use from TLS variable destructors. See source
code comments for more details.
