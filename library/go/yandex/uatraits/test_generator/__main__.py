import argparse
import json
import tempfile

import uatraits

import library.python.resource as rs


def create_temp_file_for_resource(resource_key):
    temp_file = tempfile.NamedTemporaryFile()
    temp_file.write(rs.find(resource_key))
    temp_file.flush()
    return temp_file

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Helper script for generating tests for the Go-implementation of uatraits")
    parser.add_argument("--input", type=str, help="input JSON file name which contains user agent list")
    parser.add_argument("--output", type=str, help="output JSON file name where parsed user agents should be stored")
    args = parser.parse_args()

    browser_file = create_temp_file_for_resource("browser")
    profiles_file = create_temp_file_for_resource("profiles")
    extra_file = create_temp_file_for_resource("extra")

    ua_detector = uatraits.detector(
        browser_file.name,
        profiles_file.name,
        extra_file.name
    )

    with open(args.input) as input_file:
        user_agents = json.load(input_file)

    results = []
    for user_agent in user_agents:
        traits = ua_detector.detect(str(user_agent))
        results.append({
            "user_agent": user_agent,
            "traits": traits
        })

    with open(args.output, 'w') as output_file:
        json.dump(results, output_file, indent=4)
