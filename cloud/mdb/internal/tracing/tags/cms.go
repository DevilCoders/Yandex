package tags

var (
	CmsOperationID   = StringTagName("cms.operation.id")
	CmsOperationType = StringTagName("cms.operation.type")
	CMSRequestID     = StringTagName("cms.request_id")
	DecisionID       = StringTagName("cms.decision_id")
	Dom0Fqdn         = StringTagName("dom0.fqdn")
	InstanceFQDN     = StringTagName("instance.fqdn")
	InstanceFQDNs    = StringsTagName("instance.fqdns")
	InstanceID       = StringTagName("instance.id")
	InstructionName  = StringTagName("cms.instruction.name")
	StepName         = StringTagName("cms.step.name")
	LockID           = StringTagName("lock.id")
)
