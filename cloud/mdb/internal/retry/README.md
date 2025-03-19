# Wrapper for github.com/cenkalti/backoff

### Benchmarks
To run do `go test -bench=. -benchmem`

##### Results
```
BenchmarkBackoff/Retry-12                3263481               372 ns/op             144 B/op          3 allocs/op
BenchmarkBackoff/BackOff.Retry-12        4797382               249 ns/op              32 B/op          1 allocs/op
```

