#
# BRIGDE
# by oldteam & lomaster
# License CC-BY-NC-SA 4.0
# Thanks to ......2007
#   Сделано от души 2023.
#

VERSION = "0.2"

import time
import argparse
import datetime
import requests
import nextcloud_client

consensuses = []
consensuses_ok = []

parser = argparse.ArgumentParser(description='Brigde for import and export.')

#parser.add_argument('url',  type=str, help='set host url')
parser.add_argument('-f', '--files', type=str, nargs='+', help='set list of files')
parser.add_argument('-c', '--consensuses', type=str, help='specify the file with the conseunces')
parser.add_argument('-i', '--import-file', action='store_true', help='transfer the file over the bridge')
parser.add_argument('-e', '--export-file', action='store_true', help='enter the file on the bridge')

current_datetime   = datetime.datetime.now()
formatted_datetime = current_datetime.strftime("%Y-%m-%d %H:%M")

args = parser.parse_args()

def quitting():
    print("\nStopping Bridge...")
    exit()

def parse_file_to_array(file_path):
    result_array = []
    try:
        with open(file_path, 'r') as file:
            for line in file:
                result_array.append(line.strip())
    except IOError:
        print(f"Failed read file {file_path}!")
    return result_array

def probe_http(url):
    try:
        response = requests.get(url)
        if response.status_code == 200 or response.status_code == 301:
            return True
    except requests.ConnectionError:
        return False 

print(f"Running Bridge {VERSION} at {formatted_datetime}")

if not args.import_file and not args.export_file:
    print("You did not state the reason for the hike.")
    quitting()

if args.consensuses:
    consensuses = parse_file_to_array(args.consensuses)

print(f"Loaded \"{args.consensuses}\" in which {len(consensuses)} consensuses, they've been run.")

# Checking consensuses
for c in consensuses:
    temp = probe_http(c)
    if temp:
        consensuses_ok.append(c)

print(f"Checked out {len(consensuses_ok)} successful consensuses.\n")

def import_files(url, paths):
    for path in paths:
        try:
            start_time = time.time()
            nc = nextcloud_client.Client.from_public_link(url)
            nc.drop_file(path)
            end_time = time.time()
            execution_time = end_time - start_time
            formatted_time = "{:.2f}".format(execution_time)
            print(f"{path} -> {path} File has been uploaded on {formatted_time}s")
        except nextcloud_client.nextcloud_client.HTTPResponseError as e:
            return False
    return True

def export_files(url, paths):
    for path in paths:
        try:
            start_time = time.time()
            nc = nextcloud_client.Client.from_public_link(url, password="")
            nc.get_file(path, path)
            end_time = time.time()
            execution_time = end_time - start_time
            formatted_time = "{:.2f}".format(execution_time)
            print(f"{path} <- {path} File has been received on {formatted_time}s")
        except nextcloud_client.nextcloud_client.HTTPResponseError as e:
            return False
    return True

if args.import_file and args.files:
    for c in consensuses_ok:
        print(f"* Tyring import from {c}")
        temp = import_files(c, args.files)
        if not temp:
            continue

if args.export_file and args.files:
    for c in consensuses_ok:
        print(f"* Tyring import in {c}")
        temp = export_files(c, args.files)
        if not temp:
            continue

quitting()
