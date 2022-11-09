import json
import os
import argparse

from itertools import groupby

parser = argparse.ArgumentParser()
parser.add_argument('-rgb_path', action="store", dest='rgb_path', default=0)
args = parser.parse_args()

# opens JSON file
json_file = open('../pipeline/data/smarthome_CS_51.json')

# returns JSON object as a dictionary
data = json.load(json_file)

valid_videos = []

# iterates through the json and adds each video id to valid_videos list
for id in data:
    valid_videos.append(id)

# closes JSON file
json_file.close()

extracted_video_ids = []
extraction_video_filenames = []
path = args.rgb_path

files = os.listdir(path)

# lists all files in the RGB directory and 
# saves all the video IDs into the extracted_video_ids list 
# and all the video filenames into the extraction_video_filenames list 
for f in files:
    extracted_video_ids.append(f[0:9])
    extraction_video_filenames.append(f.split('.')[0])

# groups the video filenames by the video ID
extraction_video_filenames.sort()
grouped_filenames = [list(i) for j, i in groupby(
    extraction_video_filenames, lambda a: a[0:9]
)]

# displays length of each video list
print(f"original videos in smarthome_CS_51.json: {len(valid_videos)}")
print(f"extracted videos found in '{path}': {len(extracted_video_ids)}")

extracted_set = set(extracted_video_ids)
missing_vids = []

# removes video IDs that do not have RGB npy files
for id in valid_videos:
    if id not in extracted_video_ids:
        data.pop(id)
        missing_vids.append(id)

# iterates through the grouped filenames to retrieve 
# each of the unique variation of the original video ID
# it then creates a new entry based on the variation of the video ID
# in which it duplicates the existing video ID's values
for grouped_filename in grouped_filenames:
    if len(grouped_filename) > 1:
        for filename in grouped_filename:
            video_id = filename.split('_')[0]
            if (video_id in valid_videos) and (filename != video_id):
                data[filename] = data[video_id]

# creates new updated version of smarthome.json with removed video IDs
with open('../pipeline/data/smarthome_CS_51_v2.json', "w") as outfile:
    json.dump(data, outfile)

print(f"{len(valid_videos) - len(missing_vids)}/536 extracted original videos found")
print(f"{len(missing_vids)} original videos removed from smarthome_CS_51_v2.json")
print(f"smarthome_CS_51_v2.json saved to ./pipeline/data/")
