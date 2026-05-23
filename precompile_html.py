import sys
import os
import re

SRC_DIR = "frontend/html"
FRAGMENTS_DIR = "frontend/html_fragments"
BUILD_DIR = "data/html"

def build_html():
    if not os.path.exists(BUILD_DIR):
        os.makedirs(BUILD_DIR)

    # Regex to find tags like %HEADER%
    tag_pattern = re.compile(r'%([A-Z_]+)%')

    for filename in os.listdir(SRC_DIR):
        if not filename.endswith(".html"):
            continue
        src_path = os.path.join(SRC_DIR, filename)
        dest_path = os.path.join(BUILD_DIR, filename)

        with open(src_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # Find all tags in the file
        tags = tag_pattern.findall(content)
        
        for tag in tags:
            # If the tag name matches a file in html_fragments (e.g., %HEADER% -> header.html), replace it.
            fragment_filename = f"{tag.lower()}.html"
            fragment_path = os.path.join(FRAGMENTS_DIR, fragment_filename)

            if os.path.exists(fragment_path):
                with open(fragment_path, 'r', encoding='utf-8') as frag_file:
                    fragment_content = frag_file.read()
                
                # Replace the tag with the raw HTML fragment
                content = content.replace(f"%{tag}%", fragment_content)
                print(f"{filename} - Inlined fragment: {fragment_filename}")

        # Write the fully assembled file to the data directory
        with open(dest_path, 'w', encoding='utf-8') as f:
            f.write(content)

if "buildfs" in sys.argv:
    build_html()