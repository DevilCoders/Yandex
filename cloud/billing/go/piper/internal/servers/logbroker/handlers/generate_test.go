package handlers

//go:generate mockery --disable-version-string --name MetricsResolver
//go:generate mockery --disable-version-string --name IdentityResolver
//go:generate mockery --disable-version-string --name AbcResolver
//go:generate mockery --disable-version-string --name MetricsPusher
//go:generate mockery --disable-version-string --name BillingAccountsGetter
//go:generate mockery --disable-version-string --name OversizedMessagePusher
//go:generate mockery --disable-version-string --name DumpErrorsTarget
//go:generate mockery --disable-version-string --name YDBPresenterTarget
//go:generate mockery --disable-version-string --name CumulativeCalculator
//go:generate mockery --disable-version-string --name DuplicatesSeeker
//go:generate mockery --disable-version-string --name E2EPusher

//go:generate mockery --disable-version-string --name MessagesReporter --dir ../../../../../pkg/logbroker/lbtypes
