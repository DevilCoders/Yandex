package tags

import "a.yandex-team.ru/cloud/mdb/internal/tracing/tags"

var (
	MinionFQDN            = tags.StringTagName("deploy.minion.fqdn")
	ShipmentID            = shipmentIDTagName("deploy.shipment.id")
	CommandID             = commandIDTagName("deploy.command.id")
	JobID                 = jobIDTagName("deploy.job.id")
	JobExtID              = tags.StringTagName("deploy.job.ext_id")
	DispatchCommandsCount = tags.IntTagName("deploy.dispatch.commands_count")
)
