import argparse
import ruamel.yaml


def main():
    parser = argparse.ArgumentParser(description='Extract boxes')
    parser.add_argument('-i', required=True)
    parser.add_argument('-u', required=False)
    parser.add_argument('-o', required=True)
    args = parser.parse_args()

    file_in = args.i
    deploy_unit = args.u
    file_out = args.o
    print("Loading %s" % file_in)
    if deploy_unit:
        print("Using deploy unit %s" % deploy_unit)

    yaml = ruamel.yaml.YAML()  # defaults to round-trip if no parameters given
    with open(file_in) as f:
        root = yaml.load(f)

    stage_id = root['meta']['id']

    units = root['spec']['deploy_units']

    if not deploy_unit:
        assert len(units) == 1, "Expect only single unit, found %s" % len(units)
    for unit_id in units:
        if deploy_unit and unit_id != deploy_unit:
            continue
        unit = units[unit_id]

        pod = unit['multi_cluster_replica_set']['replica_set']['pod_template_spec']['spec']
        pod_agent = pod['pod_agent_payload']
        pod_spec = pod_agent['spec']

        boxes = pod_spec['boxes']

        assert len(boxes) == 1, "Expect only single box, found %s" % len(boxes)
        for box in boxes:
            box_id = box['id']
            print("Saving %s" % file_out)
            with open(file_out, 'w') as f:
                f.write(
    '''%s)
    UNIT="%s"
    BOX="%s"
    ;;
''' % (stage_id, unit_id, box_id))


if __name__ == "__main__":
    main()
