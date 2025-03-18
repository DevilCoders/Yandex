module test.yandex/modules

require (
	github.com/buglloc/kek v1.0.1
	github.com/buglloc/mega-blah v1.0.0
	github.com/go-chi/chi v0.0.0-00010101000000-000000000000
)

replace github.com/go-chi/chi => ./packages/chi

go 1.15
