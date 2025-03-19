package tags

var (
	CNAMEFqdn           = StringTagName("fqdn.cname")
	CNAMEFqdns          = StringsTagName("fqdn.cnames")
	ConductorGroup      = StringTagName("conductor.group")
	DNSNameserver       = StringTagName("dns.nameserver")
	NewFqdn             = StringTagName("fqdn.new")
	OldFqdn             = StringTagName("fqdn.old")
	PrimaryFqdn         = StringTagName("fqdn.primary")
	SecondaryFqdn       = StringTagName("fqdn.secondary")
	TargetFqdn          = StringTagName("fqdn.target")
	UpdatePrimaryFqdn   = BoolTagName("fqdn.primary.update")
	UpdateSecondaryFqdn = BoolTagName("fqdn.secondary.update")
	DataCenter          = StringTagName("datacenter")
)
