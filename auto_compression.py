import os
import gzip
import shutil

import sys
import subprocess

try:
    import minify_html
    import rjsmin
    import rcssmin
except ImportError:
    print("Bibliotheken fehlen. Installiere minify_html, rjsmin, rcssmin...")
    subprocess.check_call([sys.executable, '-m', 'pip', 'install', 'minify_html', 'rjsmin', 'rcssmin'])
    import minify_html
    import rjsmin
    import rcssmin
    print("Installation erfolgreich!")


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
        content_to_compress = None
        
        # Check if we should minify before compressing
        if src_path.endswith((".html", ".htm")):
            with open(src_path, 'r', encoding='utf-8') as f:
                content = f.read()
            original_size = len(content)
            minified = minify_html.minify(content, minify_js=True, minify_css=True)
            content_to_compress = minified.encode('utf-8')
            print(f"Minified & Compressed {os.path.basename(src_path)}: {original_size} -> minified -> gz")
            
        elif src_path.endswith(".js") and not src_path.endswith(".min.js"):
            with open(src_path, 'r', encoding='utf-8') as f:
                content = f.read()
            original_size = len(content)
            minified = rjsmin.jsmin(content)
            content_to_compress = minified.encode('utf-8')
            print(f"Minified & Compressed {os.path.basename(src_path)}: {original_size} -> minified -> gz")
            
        elif src_path.endswith(".css") and not src_path.endswith(".min.css"):
            with open(src_path, 'r', encoding='utf-8') as f:
                content = f.read()
            original_size = len(content)
            minified = rcssmin.cssmin(content)
            content_to_compress = minified.encode('utf-8')
            print(f"Minified & Compressed {os.path.basename(src_path)}: {original_size} -> minified -> gz")
            
        if content_to_compress is not None:
            with gzip.open(dest_path, "wb") as f_out:
                f_out.write(content_to_compress)
        else:
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
        if src_path.endswith((".html", ".htm")):
            with open(src_path, 'r', encoding='utf-8') as f:
                content = f.read()
            original_size = len(content)
            minified = minify_html.minify(content, minify_js=True, minify_css=True)
            
        elif src_path.endswith(".js") and not src_path.endswith(".min.js"):
            with open(src_path, 'r', encoding='utf-8') as f:
                content = f.read()
            original_size = len(content)
            minified = rjsmin.jsmin(content)
            
        elif src_path.endswith(".css") and not src_path.endswith(".min.css"):
            with open(src_path, 'r', encoding='utf-8') as f:
                content = f.read()
            original_size = len(content)
            minified = rcssmin.cssmin(content)
            
        else:
            # Für alle anderen Dateien: Normaler Copy
            shutil.copy2(src_path, dest_path)
            return True

        # Wenn wir hier sind, wurde die Datei minifiziert (html, js oder css)
        new_size = len(minified)
        with open(dest_path, 'w', encoding='utf-8') as f:
            f.write(minified)
            
        print(f"Minified {os.path.basename(src_path)}: {original_size} -> {new_size} bytes")
        return True
            
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

