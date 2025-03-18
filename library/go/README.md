# Awesome go

# Content

- [Vendor](#vendor)
- [Build](#build)
- [Security](#security)
- [Logging](#logging)
- [Testing](#testing)
- [Monitoring](#monitoring)
- [Services](#services)
- [Power of C++](#power-of-c)
- [Algorithms](#algorithms)
- [HTTP](#http)
- [Configuration](#configuration)
- [Codecs](#codecs)
- [Protobuf](#protobuf)
- [System](#system)
- [Databases](#databases)
- [Caching](#caching)
- [CLI](#cli)
- [Text Processing](#text-processing)
- [golang/x](#golangx)
- [Video](#video)
- [Misc](#misc)

## Vendor

[vendor.policy](https://a.yandex-team.ru/arc/trunk/arcadia/build/rules/go/vendor.policy) - import rules list

## Build

*Integration with arcadia build system*

* [buildinfo](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/core/buildinfo) - get svn revision and build time
* [resource](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/core/resource) - link files into the binary
* [httputil/resource](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/httputil/resource) - expose linked resource as `http.Filesystem`
* [httputil/swaggerui](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/httputil/swaggerui) - embedded SwaggerUI resources. Allows you to serve the swagger definitions/interface directly from your app
* [yatool](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yatool) - find current `ya` binary. Useful for calling other tools
* [yatest](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/test/yatest) - access environment when running under `ya make -t`
* [recipe](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/test/recipe) - write custom ya.make recipes

## Security

*Libraries that are used to help make your application more secure.*

* [certifi](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/certifi) - collection of public and internal Root Certificates
* [tvm](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yandex/tvm) - fast and secure cryptographic tickets
* [oauth](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yandex/oauth) - create OAuth token from SSH key. Allows you to build zero-configuration CLI tools
* [blackbox](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yandex/blackbox) - yandex blackbox client
* [middleware/tvm](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/httputil/middleware/tvm) - middleware validating service tickets
* [valid](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/valid) - various data and types validation package
* [masksecret](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/masksecret) - hide sensitive data from possibly public objects

*Third-party packages*

* [jwt-go](https://github.com/dgrijalva/jwt-go) - implementation of JSON Web Tokens (JWT)
* [oauth2](https://golang.org/x/oauth2) - provides support for making OAuth2 authorized and authenticated HTTP requests
* [xxhash](https://github.com/OneOfOne/xxhash) - xxhash32 and xxhash64 hash functions
* [murmur3](https://github.com/twmb/murmur3) - fast, fully fledged murmur3 in Go
* [siphash](https://github.com/dchest/siphash) - Go implementation of SipHash-2-4
* [uuid](https://github.com/gofrs/uuid) - a pure Go implementation of Universally Unique Identifiers

## Logging

*Libraries for application logging*

* [log](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/core/log) - package with default logger interfaces and various implementations

*Third-party packages*

* [journal](https://github.com/coreos/go-systemd/journal) - logging to systemd
* [spew](https://github.com/davecgh/go-spew/spew) - pretty printer
* [zap](https://go.uber.org/zap) - default logging library. See also: library/go/core/log

## Testing

* [test](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/test) - various testing helpers and receipts

*Third-party packages*

* [testify](https://github.com/stretchr/testify) - a toolkit with common test assertions, suites and mocks
* [miniredis](https://github.com/alicebob/miniredis) - in-process redis server for tests
* [go-sqlmock](https://github.com/DATA-DOG/go-sqlmock) - sql.DB mocking library
* [godog](https://github.com/DATA-DOG/godog) - Cucumber-like BDD library
* [clockwork](https://github.com/jonboulle/clockwork) - clock mock for tests
* [goleak](https://go.uber.org/goleak) - find goroutine leaks in tests
* [mock](https://github.com/golang/mock) - mocking framework
* [pandora](https://github.com/yandex/pandora) - Go load generator. Can be used as library for custom load tools

## Monitoring

*Libraries to help you monitor application health and performance*

* [metrics](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/core/metrics) - metrics library with solomon and prometheus backends
* [httpmetrics](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/httputil/middleware/httpmetrics) - middleware measuring important metrics of HTTP service
* [unistat](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yandex/unistat) - expose metrics in unistat format

*Third-party packages*

* [prometheus_client](https://github.com/prometheus/client_golang), [prometheus_model](https://github.com/prometheus/client_model), [prometheus_common](https://github.com/prometheus/common) - prometheus client
* [uber/jaeger_client](https://github.com/uber/jaeger-client-go), [jaegertracing/jaeger-client](https://github.com/jaegertracing/jaeger-client-go), [opentracing-go](https://github.com/opentracing/opentracing-go) - Opentracing interface and implementation

## Services

*Client libraries for Yandex internal services*

* [laas](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yandex/laas) - location as a service
* [yav](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yandex/yav) - yandex vault
* [ypsd](https://a.yandex-team.ru/arc/trunk/arcadia/infra/yp_service_discovery/golang/resolver) - YP service discovery client
* [yt](https://a.yandex-team.ru/arc/trunk/arcadia/yt/go/yt) - YT client
* [ydb](https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/public/sdk/go/ydb) - YDB
* [persqueue](https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/public/sdk/go/persqueue) - like kafka but better
* [bunker](https://a.yandex-team.ru/arc/trunk/arcadia/toolbox/bunker) - bunker client
* [yp](https://a.yandex-team.ru/arc/trunk/arcadia/yp/go) - YP client

*Third-party services and APIs clients*

* [amqp](https://github.com/streadway/amqp) - AMQP client with RabbitMQ extensions
* [etcd](https://github.com/coreos/etcd) - etcd client
* [docker](https://github.com/docker/docker) - Docker client
* [zk](https://github.com/go-zookeeper/zk) - ZooKeeper client
* [statsd](https://github.com/smira/go-statsd) - statsd client library
* [ldap](https://gopkg.in/ldap.v3) - basic LDAP v3 functionality
* [aws-go](https://github.com/aws/aws-sdk-go) - AWS client libraries
* [github](https://github.com/google/go-github/v25/github) - library for accessing the GitHub API v3
* [apns2](https://github.com/sideshow/apns2) - Apple notifcation service client
* [firebase](https://firebase.google.com/go) - Google firebase library

## Power of C++

*Cgo binding for popolar and powerful C++ libraries*

* [geobase](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yandex/geobase) - find location by IP

*Third-party C/C++ bindings*

* [imagic](https://gopkg.in/gographics/imagick.v2) - bindings to ImageMagick's MagickWand C API
* [gobpf](https://github.com/iovisor/gobpf) - bindings for the bcc framework as well as low-level routines to load and use eBPF programs from .elf files

## Algorithms

*Various classic algorithms implementation*

* [discreterand](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/discreterand) - implementation of Vose's alias method for choosing elements from a discrete distribution

## HTTP

*Packages to work with HTTP*

* [headers](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/httputil/headers) - a set of constants and helpers to work with popular HTTP headers
* [middlewares](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/httputil/middleware) - useful net/http middlewares
* [render](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/httputil/render) - simplified HTTP response rendering based on content type
* [status](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/httputil/status) - HTTP statuses helpers
* [uatraits](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yandex/uatraits) - Go-native implementation of uatraits library for parsing User-Agent strings

*Third-party packages*

* [chi](https://github.com/go-chi/chi) - lightweight, idiomatic and composable router
* [echo](https://github.com/labstack/echo) - high performance, minimalist web framework
* [resty](https://github.com/go-resty/resty) - simple HTTP and REST client library
* [cors](https://github.com/rs/cors) - net/http handler implementing Cross Origin Resource Sharing W3 specification
* [websocket](https://github.com/gorilla/websocket) - a fast, well-tested and widely used WebSocket implementation for Go
* [sessions](https://github.com/gorilla/sessions) - provides cookie and filesystem sessions and infrastructure for custom session backends
* [schema](https://github.com/gorilla/schema) - Package gorilla/schema fills a struct with form values

## Configuration

* [confita](https://github.com/heetch/confita) - load configuration in cascade from multiple backends into a struct
* [toml](https://github.com/BurntSushi/toml) - TOML parser/encoder with reflection
* [yaml](https://gopkg.in/yaml.v2) - YAML support for Go
* [ini](https://gopkg.in/ini.v1) - INI format support
* [maxprocs](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/maxprocs) - automagically configure GOMAXPROCS in any deploy environment

## Codecs

* [easyjson](https://github.com/mailru/easyjson) - fast JSON serializer for golang. Relies on code generation
* [fastjson](https://github.com/valyala/fastjson) - very fast json parser and encoder
* [unknownjson](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/x/encoding/unknownjson) - simplify handling of unknown JSON fields
* [snappy](https://github.com/golang/snappy) - Snappy compression format in the Go programming language
* [lz4](https://github.com/pierrec/lz4) - lz4 compression codec
* [g711](https://github.com/zaf/g711) - encode and decode ITU-T G.711 sound data
* [protoseq](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/x/encoding/protoseq) - coding scheme for framing multiple Protobuf messages
* [zstd](https://github.com/klauspost/compress/zstd) - encode and decode ZStandard

## Protobuf

* [protobuf](https://github.com/golang/protobuf) - Go support for Google's protocol buffers
* [genproto](https://google.golang.org/genproto) - Go generated proto packages for Google services
* [grpc](https://google.golang.org/grpc) - Google RPC for protobuf
* [go-grpc-middleware](https://github.com/grpc-ecosystem/go-grpc-middleware) - gRPC Middlewares: interceptor chaining, auth, logging, retries and more
* [grpc-gateway](https://github.com/grpc-ecosystem/grpc-gateway) - generate fancy REST API from gRPC

## System

* [fsnotify](https://github.com/fsnotify/fsnotify) - cross-platform file system notifications
* [dns](https://github.com/miekg/dns) - complete and usable DNS library
* [isatty](https://github.com/mattn/go-isatty) - isatty for golang
* [go-homedir](https://github.com/mitchellh/go-homedir) - library for detecting and expanding the user's home directory without cgo
* [go-ping](https://github.com/sparrc/go-ping) - ICMP Ping library
* [serial](https://github.com/tarm/serial) - read and write from the serial port as a stream of bytes
* [gopsutil](https://github.com/shirou/gopsutil) - library for retrieving information on running processes and system utilization
* [cmd](https://github.com/go-cmd/cmd) - a wrapper around os/exec.Cmd to run external commands asynchronously (for Linux and macOS)
* [gpu-monitoring-tools](https://github.com/NVIDIA/gpu-monitoring-tools) - bindings for NVIDIA Management Library (NVML) for monitoring and managing GPU devices (Linux only)
* [dbus](https://github.com/godbus/dbus) - bindings for the D-Bus message bus system
* [osquery](https://github.com/kolide/osquery-go) - osquery exposes an operating system as a high-performance relational database
* [netlink](https://github.com/vishvananda/netlink) - netlink is the interface a user-space program in linux uses to communicate with the kernel
* [netns](https://github.com/vishvananda/netns) - package provides an ultra-simple interface for handling network namespaces
* [gosnmp](https://github.com/soniah/gosnmp) - package provides Get, GetNext, GetBulk, Walk, BulkWalk, Set and Traps. It supports IPv4 and IPv6, using SNMPv2c or SNMPv3

## Databases

*Libraries designed to simplify integration with databases*

* [hasql](https://golang.yandex/hasql) - cluster-aware wrapper for database/sql and sqlx (look for extensions [here](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/x/yandex/hasql))

*Third-party librabries and drivers*

* [mysql](https://github.com/go-sql-driver/mysql) - a pure Go MySQL driver
* [pgx](https://github.com/jackc/pgx) - PostgreSQL driver and toolkit for Go
* [clickhouse](https://github.com/ClickHouse/clickhouse-go) - driver for ClickHouse column-oriented database management system
* [mongo-driver](https://go.mongodb.org/mongo-driver) - official MongoDB driver
* [go-sqlite3](https://github.com/mattn/go-sqlite3) - sqlite3 driver for go using database/sql
* [sqlx](https://github.com/jmoiron/sqlx) - database/sql wrapper with a lot of helper functions
* [squirrel](https://github.com/Masterminds/squirrel) - fluent SQL builder
* [bbolt](https://github.com/etcd-io/bbolt) - an embedded key/value database for Go based on B+-tree
* [pgerrcode](https://github.com/jackc/pgerrcode) - constants for PostgreSQL error codes

## Caching

* [ccache](https://github.com/karlseguin/ccache) - LRU Cache with TTL
* [redis](https://github.com/go-redis/redis) - redis client
* [golang-lru](https://github.com/hashicorp/golang-lru) - fixed-size thread safe LRU cache
* [groupcache](https://github.com/golang/groupcache) - distributed caching and cache-filling library

## CLI

* [cobra](https://github.com/spf13/cobra) - library for creating CLI applications
* [pflag](https://github.com/spf13/pflag) - drop-in replacement for stdlib flag module
* [mpb](https://github.com/vbauerster/mpb) - interactive terminal progress bar


## Text Processing

* [strcase](https://github.com/iancoleman/strcase) - string case conversion library
* [goldmark](https://github.com/yuin/goldmark) - a Markdown parser. Easy to extend, standards-compliant.

## golang/x

* [net](https://golang.org/x/net) - experimental network-related packages
* [crypto](https://golang.org/x/crypto) - experimental cryptography packages
* [tools](https://golang.org/x/tools) - experimental Golang tooling
* [sync](https://golang.org/x/sync) - experimental synchronization packages
* [text](https://golang.org/x/text) - packages related to internationalization (i18n) and localization (l10n)
* [exp/errors](https://golang.org/x/exp/errors) - experimental implementation of Go error inspection. Use x/xerrors instead.
* [xerrors](https://golang.org/x/xerrors) - less experimental implementation of Go error inspection
* [sys](https://golang.org/x/sys) - golang.org/x/sys required for SO_REUSEPORT, for example

## Video

* [m3u8](https://github.com/grafov/m3u8) - parser and generator of M3U8-playlists for Apple HLS
* [astits](github.com/asticode/go-astits) - parse and demux MPEG Transport Streams
* [rtp](github.com/pion/rtp) - go implementation of RTP
* [rtsp](github.com/aler9/gortsplib) - RTSP 1.0 client and server library

## Misc

* [flock](https://github.com/gofrs/flock) - thread-safe file locking library
* [datasize](https://github.com/c2h5oh/datasize) - helpers for data sizes (kilobytes, petabytes), human readable sizes, parsing
* [copystructure](https://github.com/mitchellh/copystructure) - deep copying values
* [mapstructure](https://github.com/mitchellh/mapstructure) - decoding generic map values to structures and vice versa
* [stats](https://github.com/montanaflynn/stats) - a well tested and comprehensive Golang statistics library package with no dependencies
* [go-openapi](https://github.com/go-openapi) - runtime dependencies of go-swagger
* [goquery](https://github.com/PuerkitoBio/goquery) - library for HTML DOM manipulations and searching nodes by CSS selectors
* [hashring](https://github.com/serialx/hashring) - consistent hashing "hashring" implementation in golang
* [go-difflib](https://github.com/pmezard/go-difflib) - text diffing
* [cast](https://github.com/spf13/cast) - safe and easy casting from one type to another
* [gocron](https://github.com/jasonlvhit/gocron) - job scheduling package
* [tusd](https://github.com/tus/tusd) - tus: the open protocol for resumable file uploads
* [dateparse](https://github.com/araddon/dateparse) - parse many date strings without knowing format in advance
* [semver](https://github.com/blang/semver) - semantic versioning parsing and comparision library
* [atomic](https://go.uber.org/atomic) - simple wrappers around numerics to enforce atomic access
* [mergo](https://github.com/imdario/mergo) - merging same-type structs with exported fields initialized as zero value of their type and same-types maps
* [backoff](https://github.com/cenkalti/backoff) - exponential backoff algorithm & retry library
* [participle](https://github.com/alecthomas/participle) - this package provides a simple, idiomatic and elegant way of defining parsers in Go
* [excelize](https://github.com/360EntSecGroup-Skylar/excelize) - library for reading and writing Microsoft Excel (XLSX) files
* [godirwalk](https://github.com/karrick/godirwalk) - fast directory traversal for Golang
* [dst](https://github.com/dave/dst) - Decorated Syntax Tree - manipulate Go source with perfect fidelity
* [slack](https://github.com/slack-go/slack) - Slack API
