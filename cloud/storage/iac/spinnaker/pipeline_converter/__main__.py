import json
import click


@click.command()
@click.option("--source", help="path to test file",
              default="pipelines/default/yc-nbs-testing/Deploy.Straight.Test.json",
              type=click.Path(
                  exists=True, resolve_path=True, readable=True, dir_okay=False, file_okay=True
              )
              )
@click.option("--output", help="path to output file (id and name would be used from it)",
              default="pipelines/default/yc-nbs-testing/Deploy.Straight.json",
              type=click.Path(
                  exists=True, resolve_path=True, writable=True, file_okay=True, dir_okay=False
              )
              )
def main(source, output):
    with open(source) as fp:
        source_json = json.load(fp)

    with open(output) as fp:
        output_json = json.load(fp)

    for k in ["id", "name", "index"]:
        source_json[k] = output_json[k]

    source_json["parameterConfig"] = [i for i in source_json.get("parameterConfig", []) if i.get("name") != "test"]
    ref_ids = []
    ref_idxs = []
    stages = source_json.get("stages", [])
    for idx, stage in enumerate(stages):

        if stage.get("refId") == "1" and stage.get("type") == "evaluateVariables":
            stage["variables"] = [i for i in stage.get("variables", []) if i.get("key") != "test"]

        if stage.get("name", "").startswith("[TEST]"):
            ref_idxs.append(idx)
            ref_ids.append(stage.get("refId"))

        expression = stage.get("stageEnabled", {}).get("expression", "")
        if expression == 'test == \"no\"' or expression == 'test == \"yes\"':
            del stage["stageEnabled"]

        if stage.get("stageEnabled", {}).get("expression", ""):
            if ' && test == \"no\"' in stage["stageEnabled"]["expression"]:
                expression = expression.replace('&& &&', '&&')
                expression = expression.replace(' && test == \"no\"', '').replace(' && test == \"yes\"', '')
                stage["stageEnabled"]["expression"] = expression

    for stage in stages:
        requisite_stage_ref_ids = []
        for idx in stage.get("requisiteStageRefIds", []):
            if idx not in ref_ids:
                requisite_stage_ref_ids.append(idx)
        stage["requisiteStageRefIds"] = requisite_stage_ref_ids

    for idx in sorted(ref_idxs, reverse=True):
        stages.pop(idx)

    with open(output, "w") as fp:
        json.dump(source_json, fp, indent=True)


if __name__ == "__main__":
    main()
