package services

import (
	"context"
	"fmt"
	"time"

	"github.com/vbauerster/mpb"
	"github.com/vbauerster/mpb/decor"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
)

func waitForShipment(ctx context.Context, shipment models.Shipment, dapi deployapi.Client) (models.Shipment, error) {
	progress := mpb.New()
	bar := progress.AddBar(shipment.TotalCount,
		mpb.PrependDecorators(
			decor.Name(fmt.Sprintf("shipment %q", shipment.ID)),
			decor.CountersNoUnit("% d / %d"),
		),
		mpb.AppendDecorators(
			decor.OnComplete(decor.Elapsed(decor.ET_STYLE_MMSS), "done"),
		),
	)
	bar.IncrBy(int(shipment.DoneCount))
	doneCount := shipment.DoneCount
	for {
		var err error
		shipment, err = dapi.GetShipment(ctx, shipment.ID)
		if err != nil {
			return models.Shipment{}, err
		}
		bar.IncrBy(int(shipment.DoneCount - doneCount))
		doneCount = shipment.DoneCount
		if shipment.Status == models.ShipmentStatusInProgress {
			time.Sleep(5 * time.Second)
		} else {
			break
		}
	}
	return shipment, nil
}
