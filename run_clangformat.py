Import("env")

import subprocess
import os
import sys

def check_format_callback(*arg, **kwargs):
    return formatting_callback(arg, kwargs)

def apply_format_callback(*arg, **kwargs):
    print("Formatting Source Code")
    return formatting_callback(arg, kwargs, apply=True)

def formatting_callback(*arg, **kwargs):
    subprocess.check_call([sys.executable, "-m", "pip", "install", "clang-format"])

    folders = [env.get("PROJECT_INCLUDE_DIR"), env.get("PROJECT_SRC_DIR")]
    # Add the lib folder which is not available as env variable:
    libfolder = os.path.join(env.get("PROJECT_DIR"), "lib")
    if os.path.isdir(libfolder):
        folders.append(libfolder)

    print("Formatting" if kwargs.get("apply", False) else "Checking", "the following source dirs:", folders)
    file_list = []
    for folder in folders:
        for root, dirs, files in os.walk(folder, topdown=True):
            dirs[:] = [d for d in dirs if not d.startswith('.')]
            files = [f for f in files if not f[0] == '.']

            for file in files:
                if file.endswith( ('.c','.cpp', '.h', '.hpp', '.cc', '.cxx', '.hxx','.hh') ):
                    file_list.append(os.path.join(root, file))

    # for file in file_list:
    if env.Execute("clang-format --Werror" + (" --dry-run " if not kwargs.get("apply", False) else " ") + "-i " + " ".join(f'"{f}"' for f in file_list)):
        env.Exit(1)

env.AddCustomTarget(
    "check-format",
    None,
    check_format_callback,
    title="Check clang-format",
    description="Check Source Code Formatting")

env.AddCustomTarget(
    "format",
    None,
    apply_format_callback,
    title="Apply clang-format",
    description="Run Source Code Formatting")

