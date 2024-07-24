import defusedxml.ElementTree as ET
import json
import time
import argparse
import os
from datetime import datetime, timezone

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
            # Rename to correctly post
            status = 'passed'
        elif status == 'fail':
            failed_tests += 1
            status = 'failed'
        elif status == 'skipped':
            skipped_tests += 1
        else:
            other_tests += 1

        # Getting a message if there is one
        system_out = testcase.find('system-out')
        message = system_out.text if system_out is not None else ''

        test_case = {
            "name": testcase.get('name'),
            "status": status,
            "duration": int(float(testcase.get('time', 0)) * 1000),  # Convert to milliseconds, default to 0 if not present
            "message": message
        }
        test_cases.append(test_case)

    # Get the 'timestamp' attribute of the 'testsuite' element
    timestamp_str = root.get('timestamp')
    if timestamp_str:
        # Convert the timestamp string to a datetime object
        timestamp_dt = datetime.strptime(timestamp_str, '%Y-%m-%dT%H:%M:%S')

        # Convert the datetime object to seconds since the epoch
        timestamp_seconds = int(timestamp_dt.replace(tzinfo=timezone.utc).timestamp())
    else:
        # Default timestamp if not present
        timestamp_seconds = int(datetime.now(timezone.utc).timestamp())

    # Get the current time in seconds since the epoch
    current_time_seconds = int(datetime.now(timezone.utc).timestamp())

    # Construct the summary
    summary = {
        "tests": total_tests,
        "passed": passed_tests,
        "failed": failed_tests,
        "pending": pending_tests,
        "skipped": skipped_tests,
        "other": other_tests,
        "start": timestamp_seconds,
        "stop": current_time_seconds
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

def json_to_discord_json(json_data, os_name, compiler, event):
    summary = json_data["results"]["summary"]
    tests = json_data["results"]["tests"]

    duration_seconds = summary["stop"] - summary["start"]
    duration_str = time.strftime("%H:%M:%S", time.gmtime(duration_seconds))

    # Handle empty test cases list
    name_width = max((len(test['name']) for test in tests), default=0) + 2
    status_width = max((len(test['status']) for test in tests), default=0) + 2
    duration_width = len("Duration (ms)") + 2
    flaky_width = len("Flaky 🍂") + 2

    detailed_results = "```\n"
    detailed_results += f"{'Name':<{name_width}} {'Status':<{status_width}} {'Duration (ms)':<{duration_width}} {'Flaky 🍂':<{flaky_width}}\n"
    detailed_results += "-" * (name_width + status_width + duration_width + flaky_width) + "\n"

    for test in tests:
        detailed_results += f"{test['name']:<{name_width}} {test['status']:<{status_width}} {test['duration']:<{duration_width}} {'':<{flaky_width}}\n"

    detailed_results += "```\n"

    if summary["failed"] == 0:
        failed_tests_summary = "No failed tests ✨\n"
    else:
        failed_tests_summary = "```\n"
        for test in tests:
            if test["status"] == "failed":
                failed_tests_summary += f"{test['name']}\n"
                if 'message' in test:
                    failed_tests_summary += f"{test['message'].strip()}\n"
        failed_tests_summary += "```\n"

    content = f"Test Results for **{os_name}-{compiler}** triggered by **{event}**"
    embeds = [
        {
            "title": "Test Summary",
            "description": (
                f"**Tests 📝**:    {summary['tests']}\n"
                f"**Passed ✅**:   {summary['passed']}\n"
                f"**Failed ❌**:   {summary['failed']}\n"
                f"**Skipped ⏭️**: {summary['skipped']}\n"
                f"**Pending ⏳**:  {summary['pending']}\n"
                f"**Other ❓**:    {summary['other']}\n"
                f"**Duration ⏱️**: {duration_str}\n"
            )
        },
        {
            "title": "Detailed Test Results",
            "description": detailed_results
        },
        {
            "title": "Failed Test Summary",
            "description": failed_tests_summary
        }
    ]

    discord_json = {
        "content": content,
        "embeds": embeds
    }

    return discord_json

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Convert XML test results to JSON and Discord markdown.')
    parser.add_argument('xml_file', help='Path to the XML file.')
    parser.add_argument('tool_name', help='Name of the testing tool.')
    parser.add_argument('--json_output', help='Path to the output JSON file.')
    parser.add_argument('--discord_json_output', help='Path to the output Discord JSON file.')
    parser.add_argument('--os', help="Runners operating system")
    parser.add_argument('--compiler', help="Runners compiler")
    parser.add_argument('--event', help="The event triggering the action")

    args = parser.parse_args()

    try:
        # Generate JSON data
        json_data = xml_to_json(args.xml_file, args.tool_name)

        # Determine output file paths
        json_output_file = args.json_output or f"{os.path.splitext(os.path.basename(args.xml_file))[0]}_output.json"
        discord_json_output_file = args.discord_json_output or f"{os.path.splitext(os.path.basename(args.xml_file))[0]}_discord_output.json"

        # Ensure the output directories exist
        os.makedirs(os.path.dirname(json_output_file), exist_ok=True)
        os.makedirs(os.path.dirname(discord_json_output_file), exist_ok=True)

        # Write JSON output to file
        with open(json_output_file, 'w') as json_file:
            json.dump(json_data, json_file, indent=2)
        print(f"JSON output has been written to {json_output_file}")

        # Generate Discord JSON output
        discord_json = json_to_discord_json(json_data, args.os, args.compiler, args.event)

        # Write Discord JSON output to file
        with open(discord_json_output_file, 'w') as discord_json_file:
            json.dump(discord_json, discord_json_file, indent=2)
        print(f"Discord JSON output has been written to {discord_json_output_file}")

    except FileNotFoundError as e:
        print(e)
    except Exception as e:
        print(f"An error occurred: {e}")
