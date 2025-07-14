import os
import gzip
import shutil

import sys


"""
This script compresses specific files from the frontend directory into the data directory.
Files listed in FILES_TO_COMPRESS will be compressed using gzip and saved with a .gz extension.
Other files will be copied as-is to the data directory.
TODO: Handle the files which are templated.
"""

FILES_TO_COMPRESS = [
    "css/bootstrap-5.2.3.min.css",
    "css/fontawesome-6.2.1.min.css",
    "css/uPlot.min.css",
    "js/app.js",
    "js/vue.3.2.47.min.js",
    "js/bootstrap.bundle.5.2.3.min.js",
    "js/uPlot.1.6.28.min.js",
    "js/vue-number-input.min.js",
    "js/temp.js",
    "webfonts/fa-solid-900.woff2",
    "webfonts/fa-regular-400.woff2",
]

FRONTEND_DIR = "frontend"
DATA_DIR = "data"

def ensure_dir_exists(path):
    try:
        os.makedirs(path, exist_ok=True)
    except OSError as e:
        print(f"Error creating directory {path}: {e}")

def compress_file(src_path, dest_path):
    try:
        with open(src_path, "rb") as f_in, gzip.open(dest_path, "wb") as f_out:
            shutil.copyfileobj(f_in, f_out)
    except (IOError, OSError) as e:
        print(f"Error compressing {src_path}: {e}")

        # Clean up partial file
        if os.path.exists(dest_path):
            try:
                os.remove(dest_path)
            except OSError:
                pass
        return False
    return True

def copy_file(src_path, dest_path):
    try:
        shutil.copy2(src_path, dest_path)
    except (IOError, OSError) as e:
        print(f"Error copying {src_path}: {e}")
        return False
    return True

def main():
    compress_set = set(FILES_TO_COMPRESS)
    found_files = set()

    for root, dirs, files in os.walk(FRONTEND_DIR):
        for file in files:
            rel_dir = os.path.relpath(root, FRONTEND_DIR)
            rel_file = os.path.join(rel_dir, file) if rel_dir != "." else file
            rel_file = rel_file.replace(os.sep, "/")
            found_files.add(rel_file)

            src_path = os.path.join(root, file)

            if rel_file in compress_set:
                dest_file = rel_file + ".gz"
                dest_path = os.path.join(DATA_DIR, dest_file)
                print(f"Compressing {rel_file} -> {dest_file}")
                ensure_dir_exists(os.path.dirname(dest_path))
                compress_file(src_path, dest_path)
            else:
                dest_path = os.path.join(DATA_DIR, rel_file)
                print(f"Copying {rel_file}")
                ensure_dir_exists(os.path.dirname(dest_path))
                copy_file(src_path, dest_path)

    # Check for missing files
    missing_files = compress_set - found_files

    if missing_files:
        print(f"Warning: The following files were not found: {missing_files}")

if "buildfs" in sys.argv:
    main()

if os.environ.get("PROJECT_TASK") == "buildfs":
    main()

