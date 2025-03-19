## Configuration generator
This tool was created to help build terraform configuration of existing infrastructure.
Its work is based on the values of ycp-tool output.

### Usage
#### Instances
    # create input data
    ycp compute instance list --format json > ins.json

    # generate instance configs
    go run ./main.go -i ins.json > ../../preprod/vms.tf

    # generate instance import-commands
    go run ./main.go -i ins.json -t instance-imports
#### Target groups
    # input
    ycp --profile preprod-cp load-balancer target-group list --format json > tg.json

    # configs
    go run ./main.go -i tg.json -t target-group -e preprod > ../../preprod/tgs.tf

    # imports
    go run ./main.go -i tg.json -t target-group-imports > ../../preprod/import_tgs.sh
