package entities

//go:generate stringer -type=UsageType,MetricFailReason,MatchRuleType,ResourceBindingType,UsagePeriodType -linecomment -output enums_string.go

type UsageType uint8

const (
	DeltaUsage      UsageType = iota // delta
	CumulativeUsage                  // cumulative
	GaugeUsage                       // gauge

	UnknownUsage UsageType = 255 // unknown
)

type MetricFailReason uint8

const (
	FailedByBillingAccountResolving MetricFailReason = iota // billing_account_resolving
	FailedByExpired                                         // expired
	FailedByFinishedAfterWrite                              // finish_gt_msg_write_ts
	FailedByInvalidModel                                    // invalid_model
	FailedByInvalidTags                                     // invalid_tags
	FailedByNegativeQuantity                                // negative_quantity
	FailedBySkuResolving                                    // sku_resolving
	FailedByUnparsedJSON                                    // unparsed_json
	FailedByTooBigChunk                                     // too_big_chunk
	FailedByDuplicate                                       // duplicate
)

type MatchRuleType uint8

const (
	ExistenceRule MatchRuleType = iota // exists
	ValueRule                          // value
)

type ResourceBindingType uint8

const (
	NoResourceBinding      ResourceBindingType = iota // no_binding
	TrackerResourceBinding                            // tracker
)

type UsagePeriodType uint8

const (
	UnknownPeriod UsagePeriodType = iota // unknown
	MonthlyUsage                         // monthly
)
