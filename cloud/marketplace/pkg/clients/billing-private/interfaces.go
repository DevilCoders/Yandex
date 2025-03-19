package billing

type APISession interface {
	billingAccountManager
	skuManager
}
