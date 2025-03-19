package e2e

import (
	"context"
	"strings"
	"time"

	"github.com/yandex-cloud/go-genproto/yandex/cloud/compute/v1"
	ycsdk "github.com/yandex-cloud/go-sdk"
)

func newYCClient(ctx context.Context, token string) (*ycsdk.SDK, error) {
	sdkCreds := ycsdk.NewIAMTokenCredentials(token)

	cfg := ycsdk.Config{
		Credentials: sdkCreds,
	}

	return ycsdk.Build(ctx, cfg)
}

// computeInstance holds instance itself and yc client, so we could make neat helpers.
type computeInstance struct {
	*compute.Instance
	yc *ycsdk.SDK
}

// ycTimeout is amount of time allowed to wait when making ycsdk request.
const ycTimeout = time.Second * 60

func lookupComputeInstance(yc *ycsdk.SDK, id string) (instance *computeInstance, err error) {
	r := &compute.GetInstanceRequest{
		InstanceId: id,
		View:       compute.InstanceView_FULL,
	}
	ctx, cancel := context.WithTimeout(context.Background(), ycTimeout)
	defer cancel()

	var i *compute.Instance
	i, err = yc.Compute().Instance().Get(ctx, r)
	if err != nil {
		return
	}

	instance = &computeInstance{
		Instance: i,
		yc:       yc,
	}

	return
}

// comPortNumber is port number at which agent sends its responses.
const comPortNumber = 4

// getSerialPortOutput return COM4 serial port output.
func (ci *computeInstance) getSerialPortOutput() (output []string, err error) {
	ctx, cancel := context.WithTimeout(context.Background(), ycTimeout)
	defer cancel()

	var s *compute.GetInstanceSerialPortOutputResponse
	s, err = ci.yc.Compute().Instance().GetSerialPortOutput(ctx,
		&compute.GetInstanceSerialPortOutputRequest{
			InstanceId: ci.Id,
			Port:       comPortNumber,
		})
	if err != nil {
		return
	}
	output = strings.Split(s.Contents, "\n")

	return
}

func (ci *computeInstance) updateMetadata(key, value string) (err error) {
	ctx, cancel := context.WithTimeout(context.Background(), ycTimeout)
	defer cancel()

	o, err := ci.yc.WrapOperation(ci.yc.Compute().Instance().UpdateMetadata(
		ctx,
		&compute.UpdateInstanceMetadataRequest{
			InstanceId: ci.Id,
			Upsert: map[string]string{
				key: value,
			},
		},
	))
	if err != nil {
		return
	}

	err = o.Wait(ctx)

	return
}
