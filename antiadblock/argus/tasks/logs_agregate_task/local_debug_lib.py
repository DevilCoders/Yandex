import json


def get_cases_from_configs_api(input_file):
    with open(input_file, 'r') as f:
        json_data = json.load(f)
    results = []
    for item in json_data["cases"]:
        results.append({
            "id": json_data["sandbox_id"],
            "logs": item["logs"],
            "adb_bits": item["adb_bits"],
            "request_id": item["headers"]["x-aab-requestid"],
        })
    return results
