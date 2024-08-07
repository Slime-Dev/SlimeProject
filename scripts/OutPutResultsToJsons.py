import defusedxml.ElementTree as ET
import json
import time
import argparse
import os
from PIL import Image, ImageDraw, ImageFont
from datetime import datetime, timezone
import re

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

def json_to_discord_json(json_data, os_name, compiler, event, author, branch):
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
                message = test.get('message', 'Failed test did not return a message')
                if message is not None:
                    failed_tests_summary += f"{message.strip()}\n"
                else:
                    failed_tests_summary += "Failed test did not return a message\n"
        failed_tests_summary += "```\n"

    content = f"# Test Results\n **Platform**: {os_name}-{compiler}\n**Tester:** {author}\n **Branch:** {branch}\n **Event:** {event}"
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

def parse_test_results_from_file(file_path):
    tree = ET.parse(file_path)
    root = tree.getroot()
    test_suite_name = root.attrib['name']
    tests = int(root.attrib['tests'])
    failures = int(root.attrib['failures'])
    skipped = int(root.attrib['skipped'])
    total_time = float(root.attrib['time'])

    test_cases = []
    failed_cases = []
    for testcase in root.findall('testcase'):
        name = testcase.attrib['name']
        time = float(testcase.attrib['time'])
        status = 'passed'
        error_message = None

        failure = testcase.find('failure')
        if failure is not None:
            status = 'failed'
            error_message = failure.attrib.get('message', '') + ' ' + testcase.find('system-out').text.strip()
            failed_cases.append((name, error_message))

        test_cases.append((name, status, time, error_message))

    return test_suite_name, tests, failures, skipped, total_time, test_cases, failed_cases

def create_horizontal_test_results_image(file_path, os, compiler, event, author, branch, commit_message):
    # Parse the XML data from the file
    test_suite_name, tests, failures, skipped, total_time, test_cases, failed_cases = parse_test_results_from_file(file_path)

    # Set height depending if there are failed tests
    default_height = 0
    if failures > 0:
        default_height = 250
    else:
        default_height = 200

    # Set up image
    width, height = 800, default_height + (25 * len(test_cases)) + (25 * len(failed_cases))  # Adjust height based on number of test cases and failed cases
    background_color = (45, 47, 49)  # Dark gray
    image = Image.new('RGB', (width, height), color=background_color)
    draw = ImageDraw.Draw(image)

    # Global levels
    title_height = 20
    sub_heading_height = 100
    content_buffer = 25
    line_buffer = 20
    event_spacing = 150
    left_buffer = 20
    # Load fonts
    try:
        if os == "Windows":
            title_font = ImageFont.truetype("arial.ttf", 24)
            header_font = ImageFont.truetype("arial.ttf", 18)
            body_font = ImageFont.truetype("arial.ttf", 14)
        else:
            # Use DejaVuSans as it is commonly available across platforms
            title_font = ImageFont.truetype("DejaVuSans-Bold.ttf", 24)
            header_font = ImageFont.truetype("DejaVuSans.ttf", 18)
            body_font = ImageFont.truetype("DejaVuSans.ttf", 14)
    except IOError:
        # Fallback to default font if not available
        title_font = ImageFont.load_default()
        header_font = ImageFont.load_default()
        body_font = ImageFont.load_default()

    # Colors
    white = (255, 255, 255)
    light_gray = (200, 200, 200)
    green = (0, 255, 0)
    red = (255, 0, 0)

    # Draw title
    draw.text((width / 2.5, title_height), "Test Results", font=title_font, fill=white)

    # Draw Event details
    x = left_buffer
    y = title_height + content_buffer
    draw.text((x, y), f"Author: {author}", font=body_font, fill=white)
    x += event_spacing
    draw.text((x, y), f"Event: {event}", font=body_font, fill=white)
    x += event_spacing
    draw.text((x, y), f"Branch: {branch}", font=body_font, fill=white)
    # Hard set the suite name
    test_suite_name = f"{os}-{compiler}"
    # Commit msg
    draw.text((left_buffer, y + line_buffer), f"Commit Message: {commit_message}")
    # Draw test suite information

    y = sub_heading_height
    draw.text((left_buffer, sub_heading_height), f"Details", font=header_font, fill=white)
    y += content_buffer
    draw.text((left_buffer, y), f"Test Suite: {test_suite_name}", font=body_font, fill=white)
    y += line_buffer
    draw.text((left_buffer, y), f"Total Tests: {tests}", font=body_font, fill=white)
    y += line_buffer
    draw.text((left_buffer, y), f"Failures: {failures}", font=body_font, fill=red if int(failures) > 0 else green)
    y += line_buffer
    draw.text((left_buffer, y), f"Skipped: {skipped}", font=body_font, fill=light_gray)
    y += line_buffer
    draw.text((left_buffer, y), f"Duration: {total_time:.6f}", font=body_font, fill=light_gray)

    if failures > 0:
        # Draw failed test summary header
        y += 40
        draw.text((20, y), "Failed Test Summary", font=header_font, fill=white)

        # Draw failed test cases
        y += 30
        for name, error_message in failed_cases:
            draw.text((20, y), name, font=body_font, fill=white)
            y += 25
            draw.text((20, y), error_message, font=body_font, fill=red)
            y += 25

    # Draw detailed test results header
    draw.text((400, sub_heading_height), "Detailed Test Results", font=header_font, fill=white)

    # Draw test cases
    y = sub_heading_height + content_buffer
    for name, status, duration, error_message in test_cases:
        draw.text((400, y), name, font=body_font, fill=white)
        status_color = green if status == 'passed' else red
        draw.text((580, y), status, font=body_font, fill=status_color)
        draw.text((660, y), f"{duration:.6f}", font=body_font, fill=light_gray)
        y += line_buffer

    return image

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Convert XML test results to JSON and Discord markdown.')
    parser.add_argument('xml_file', help='Path to the XML file.')
    parser.add_argument('tool_name', help='Name of the testing tool.')
    parser.add_argument('--json_output', help='Path to the output JSON file.')
    parser.add_argument('--discord_json_output', help='Path to the output Discord JSON file.')
    parser.add_argument('--image_out', help="Image to post to discord")
    parser.add_argument('--os', help="Runners operating system")
    parser.add_argument('--compiler', help="Runners compiler")
    parser.add_argument('--event', help="The event triggering the action")
    parser.add_argument('--author', help="The user triggering the action")
    parser.add_argument('--branch', help="The branch branch the action was triggered on")
    parser.add_argument('--commit_msg', help="Commit message")

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
        discord_json = json_to_discord_json(json_data, args.os, args.compiler, args.event, args.author, args.branch)

        # Write Discord JSON output to file
        with open(discord_json_output_file, 'w') as discord_json_file:
            json.dump(discord_json, discord_json_file, indent=2)
        print(f"Discord JSON output has been written to {discord_json_output_file}")

        # Generate data
        image = create_horizontal_test_results_image(args.xml_file, args.os, args.compiler, args.event, args.author, args.branch, args.commit_msg)

        # Create the image
        os.makedirs(os.path.dirname(args.image_out), exist_ok=True)
        image.save(args.image_out)
        exit(0)
    except FileNotFoundError as e:
        print(e)
        exit(1)
    except Exception as e:
        print(f"An error occurred: {e}")
        exit(1)