import defusedxml.ElementTree as ET
import json
import time
import argparse
import os

def xml_to_json(xml_file, tool_name):
    # Check if the file exists
    if not os.path.exists(xml_file):
        raise FileNotFoundError(f"The file {xml_file} does not exist.")

    # Parse the XML file securely
    tree = ET.parse(xml_file)
    root = tree.getroot()

    # Initialize summary counts
    total_tests = 0
    passed_tests = 0
    failed_tests = 0
    pending_tests = 0
    skipped_tests = 0
    other_tests = 0

    # Extract test cases
    test_cases = []
    for testcase in root.findall('testcase'):
        total_tests += 1
        status = testcase.get('status', 'unknown')
        if status == 'run':
            passed_tests += 1
        elif status == 'failed':
            failed_tests += 1
        elif status == 'skipped':
            skipped_tests += 1
        else:
            other_tests += 1

        test_case = {
            "name": testcase.get('name'),
            "status": status,
            "duration": int(float(testcase.get('time')) * 1000)  # Convert to milliseconds
        }
        test_cases.append(test_case)

    # Get current time for start and stop (example purposes, should be replaced with actual test times)
    current_time = int(time.time())

    # Construct the summary
    summary = {
        "tests": total_tests,
        "passed": passed_tests,
        "failed": failed_tests,
        "pending": pending_tests,
        "skipped": skipped_tests,
        "other": other_tests,
        "start": current_time - 20,  # Example start time
        "stop": current_time  # Example stop time
    }

    # Construct the JSON structure
    json_structure = {
        "results": {
            "tool": {
                "name": tool_name
            },
            "summary": summary,
            "tests": test_cases
        }
    }

    return json_structure

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Convert XML test results to JSON.')
    parser.add_argument('xml_file', help='Path to the XML file.')
    parser.add_argument('tool_name', help='Name of the testing tool.')
    parser.add_argument('--output', default='output.json', help='Output JSON file name (default: output.json)')

    args = parser.parse_args()

    try:
        json_data = xml_to_json(args.xml_file, args.tool_name)
        with open(args.output, 'w') as json_file:
            json.dump(json_data, json_file, indent=2)
        print(f"JSON output has been written to {args.output}")
    except FileNotFoundError as e:
        print(e)
