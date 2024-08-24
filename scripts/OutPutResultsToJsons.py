import defusedxml.ElementTree as ET
import json
import time
import argparse
import os
from PIL import Image, ImageDraw, ImageFont
from datetime import datetime, timezone
import traceback
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

def insert_newlines_after_max_length(commit_message, max_length=75):
    # Find the position of the last newline character
    last_newline_pos = commit_message.rfind('\n')

    # If there are no newlines, start from the beginning of the string
    if last_newline_pos == -1:
        start_pos = 0
    else:
        start_pos = last_newline_pos + 1

    # Process the part of the string after the last newline
    post_last_newline = commit_message[start_pos:]

    # Split the string into words
    words = post_last_newline.split()

    current_length = 0
    new_message = commit_message[:start_pos]  # Start with the part before or including the last newline

    for word in words:
        # If adding the next word would exceed the max_length, add a newline
        if current_length + len(word) + 1 > max_length:  # +1 for the space
            new_message += '\n'
            current_length = 0  # Reset the line length counter

        # Add the word to the new message
        new_message += word + ' '
        current_length += len(word) + 1  # +1 for the space

    return new_message.rstrip()  # Remove any trailing spaces

def parse_test_results_from_file(file_path):
    tree = ET.parse(file_path)
    root = tree.getroot()
    test_suite_name = root.attrib['name']
    tests = int(root.attrib['tests'])
    failures = int(root.attrib['failures'])
    skipped = int(root.attrib['skipped'])

    total_time = 0
    test_cases = []
    failed_cases = []
    total_case_lines = 0
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
            total_case_lines += len(error_message.split("\n"))
        test_cases.append((name, status, time, error_message))

        total_time += time

    return test_suite_name, tests, failures, skipped, total_time, test_cases, failed_cases, total_case_lines

def clean_log_text(log_text):
    # Remove the first "Failed" and all timestamps
    cleaned_text = re.sub(r'^Failed \[.*?\]', '', log_text)
    cleaned_text = re.sub(r'\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}\]', '', cleaned_text)

    # Remove lines containing '[info]'
    cleaned_text = re.sub(r'\[info\].*?(?=\[error\]|\Z)', '', cleaned_text, flags=re.DOTALL)

    # Remove extra spaces and newlines
    cleaned_text = re.sub(r'\s+', ' ', cleaned_text).strip()
    cleaned_text = re.sub(r'(\[error\])', r'\n\1', cleaned_text)

    return cleaned_text.strip()

def create_horizontal_test_results_image(file_path, os, compiler, event, author, branch, commit_message):
#region Image generation
    # Parse the XML data from the file
    test_suite_name, tests, failures, skipped, total_time, test_cases, failed_cases, total_case_lines = parse_test_results_from_file(file_path)

    # Manually setting the suite name as it is incorrect from the XML
    test_suite_name = compiler

    commit_message = insert_newlines_after_max_length(commit_message)
    num_new_lines = commit_message.count('\n')
    print(f"Number of new lines: {num_new_lines}")
    print(f'Number of failed test lines: {total_case_lines}')
    # Set height depending on the amount of new lines in the comment
    default_height = 475  # Base height
    height_per_line = 20  # Additional height per new line

    # Calculate the total height
    total_height = default_height + (num_new_lines * height_per_line)

    if failures != 0:
        total_height = total_height + 75 + (total_case_lines * height_per_line)

    print(f"Calculated image height: {total_height}")

    # Set up image
    width, height = 850, total_height  # Adjust height based on number of test cases and failed cases
    background_color = (0,0,0)  # black
    shading_color = (55, 57, 59)  # Slightly lighter shade for background
    #gradient_color = (50, 50, 150)  # Gradient color
    image = Image.new('RGB', (width, height), color=background_color)
    draw = ImageDraw.Draw(image)

    # Global levels
    title_height = 30
    sub_heading_height = 140
    comment_height = 350
    content_buffer = 30
    line_buffer = 25
    event_spacing = 180
    left_buffer = 30
    padding = 15  # Padding around shaded areas
    border_radius = 20
    content_box_height = 300
    content_box_width = 400
    column_buffer = width // 2.08
    title_box_width = content_box_width * 2.03
    title_box_height = sub_heading_height - line_buffer - 20

    # Load fonts
    try:
        if os == "Windows":
            title_font = ImageFont.truetype("arial.ttf", 28)
            header_font = ImageFont.truetype("arial.ttf", 20)
            body_font = ImageFont.truetype("arial.ttf", 16)
        else:
            # Use DejaVuSans as it is commonly available across platforms
            title_font = ImageFont.truetype("DejaVuSans-Bold.ttf", 28)
            header_font = ImageFont.truetype("DejaVuSans.ttf", 20)
            body_font = ImageFont.truetype("DejaVuSans.ttf", 16)
    except IOError:
        # Fallback to default font if not available
        title_font = ImageFont.load_default()
        header_font = ImageFont.load_default()
        body_font = ImageFont.load_default()

    # Colors
    white = (255, 255, 255)
    light_gray = (200, 200, 200)
    green = (0, 255, 100)
    red = (255, 100, 100)
    shadow_color = (0,0,0)
    if failures > 0:
        shadow_color = (255, 100, 100)
    else:
        shadow_color = (0, 255, 100)

    # Draw rounded rectangle with shadow for event details
    event_details_box = [left_buffer - padding, title_height - padding, title_box_width, title_box_height]
    shadow_box = [left_buffer - padding + 5, title_height - padding + 5, title_box_width + 5, title_box_height + 5]
    draw.rounded_rectangle(shadow_box, fill=shadow_color, radius=border_radius)
    draw.rounded_rectangle(event_details_box, fill=shading_color, radius=border_radius)

    # Draw title with drop shadow
    title_position = (width // 2 - 100, title_height)
    draw.text(title_position, "Test Results", font=title_font, fill=white)

    # Draw Event details text
    x = left_buffer
    y = title_height + content_buffer
    draw.text((x, y), f"Author: {author}", font=body_font, fill=white)
    x += event_spacing
    draw.text((x, y), f"Event: {event}", font=body_font, fill=white)
    x += event_spacing
    draw.text((x, y), f"Branch: {branch}", font=body_font, fill=white)

    # Draw rounded rectangle with shadow for Details section
    details_box = [left_buffer - padding, sub_heading_height - padding, content_box_width, content_box_height]
    shadow_box = [left_buffer - padding + 5, sub_heading_height - padding + 5, content_box_width + 5, content_box_height + 5]
    draw.rounded_rectangle(shadow_box, fill=shadow_color, radius=border_radius)
    draw.rounded_rectangle(details_box, fill=shading_color, radius=border_radius)

    # Draw Details text
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

    # Draw rounded rectangle with shadow for Detailed Test Results section
    detailed_results_box = [width // 2 - padding, sub_heading_height - padding, (width // 2 - padding) + content_box_width, content_box_height]
    shadow_box = [width // 2 - padding + 5, sub_heading_height - padding + 5, (width // 2 - padding) + content_box_width + 5, content_box_height + 5]
    draw.rounded_rectangle(shadow_box, fill=shadow_color, radius=border_radius)
    draw.rounded_rectangle(detailed_results_box, fill=shading_color, radius=border_radius)

    # Draw Detailed Test Results text
    y = sub_heading_height
    draw.text((column_buffer + 20, sub_heading_height), "Detailed Test Results", font=header_font, fill=white)
    y += content_buffer
    for name, status, duration, error_message in test_cases:
        draw.text((column_buffer + 20, y), name, font=body_font, fill=white)
        status_color = green if status == 'passed' else red
        draw.text((column_buffer + 200, y), status, font=body_font, fill=status_color)
        draw.text((column_buffer + 300, y), f"{duration:.6f}", font=body_font, fill=light_gray)
        y += line_buffer

    # Adding the commit msg
    # Draw rounded rectangle with a shadow for commit message
    dynamic_height = comment_height + title_box_height + (num_new_lines * height_per_line)
    print(f"Calculated dynamic box height {dynamic_height}")
    comment_box = [left_buffer - padding, comment_height - padding, title_box_width, dynamic_height]
    shadow_box = [left_buffer - padding + 5, comment_height - padding + 5, title_box_width + 5, dynamic_height + 5]
    draw.rounded_rectangle(shadow_box, fill=shadow_color, radius=border_radius)
    draw.rounded_rectangle(comment_box, fill=shading_color, radius=border_radius)

    # Draw Commit comment text
    y = comment_height
    draw.text((left_buffer, y), "Commit Comment", font=header_font, fill=white)
    y += content_buffer
    draw.text((left_buffer, y), f"{commit_message}", font=body_font, fill=white)
#endregion
    # Only draw detailed failed tests if there are any
    if failures != 0:
        dynamic_height = comment_height + (total_case_lines * height_per_line)
        failure_box_height = y + content_buffer + 75
        detailed_failure_box = [left_buffer - padding, failure_box_height - padding, title_box_width, dynamic_height + 150]
        shadow_box = [left_buffer - padding + 5, failure_box_height - padding + 5, title_box_width + 5, dynamic_height + 155]

        draw.rounded_rectangle(shadow_box, fill=shadow_color, radius=border_radius)
        draw.rounded_rectangle(detailed_failure_box, fill=shading_color, radius=border_radius)
        draw.text((left_buffer, failure_box_height), "Detailed Failed Results", font=header_font, fill=white)

        y = failure_box_height + content_buffer
        for name, status, duration, error_message in test_cases:
            if status == 'failed':
                # Draw the test name
                draw.text((left_buffer, y), name, font=body_font, fill=red)

                # Clean and wrap the error message to fit within the box width
                cleaned_text = clean_log_text(error_message)
                cleaned_texts = cleaned_text.split("\n")
                for line in cleaned_texts:
                    draw.text((left_buffer + 150, y), line, font=body_font, fill=white)
                    y += height_per_line

                y += content_buffer  # Add some buffer before the next entry


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
        # # Generate JSON data
        # json_data = xml_to_json(args.xml_file, args.tool_name)

        # # Determine output file paths
        # json_output_file = args.json_output or f"{os.path.splitext(os.path.basename(args.xml_file))[0]}_output.json"
        # discord_json_output_file = args.discord_json_output or f"{os.path.splitext(os.path.basename(args.xml_file))[0]}_discord_output.json"

        # # Ensure the output directories exist
        # os.makedirs(os.path.dirname(json_output_file), exist_ok=True)
        # os.makedirs(os.path.dirname(discord_json_output_file), exist_ok=True)

        # # Write JSON output to file
        # with open(json_output_file, 'w') as json_file:
        #     json.dump(json_data, json_file, indent=2)
        # print(f"JSON output has been written to {json_output_file}")

        # # Generate Discord JSON output
        # discord_json = json_to_discord_json(json_data, args.os, args.compiler, args.event, args.author, args.branch)

        # # Write Discord JSON output to file
        # with open(discord_json_output_file, 'w') as discord_json_file:
        #     json.dump(discord_json, discord_json_file, indent=2)
        # print(f"Discord JSON output has been written to {discord_json_output_file}")

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
        tb = traceback.extract_tb(e.__traceback__)  # Extract traceback details
        for frame in tb:
            filename = frame.filename
            lineno = frame.lineno
            function_name = frame.name
            line = frame.line
            print(f"File: {filename}, Line: {lineno}, Function: {function_name}")
            print(f"Code: {line}")
        print(f"An error occurred: {e}")
        exit(1)
