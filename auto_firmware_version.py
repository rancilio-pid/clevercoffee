###
# CleverCoffee Auto-version Script
###

import importlib.metadata
import os

# noinspection PyUnresolvedReferences
Import("env")

required_pkgs = {'dulwich'}
installed_pkgs = {dist.metadata['Name'] for dist in importlib.metadata.distributions()}
missing_pkgs = required_pkgs - installed_pkgs

if missing_pkgs:
    env.Execute('$PYTHONEXE -m pip install dulwich --global-option="--pure"')

from dulwich.repo import Repo

def get_version_build_flag() -> str:
    r = Repo('.')

    # Get the git commit hash
    commit_hash = r.head().decode("utf-8")[0:7]

    # Read version from VERSION.txt file
    version_file = "VERSION.txt"
    if os.path.exists(version_file):
        with open(version_file, 'r') as f:
            version_string = f.read().strip()
    else:
        version_string = "unknown"
        print(f"Warning: {version_file} not found, using 'unknown' as version")

    # Combine version string with commit hash
    build_version = f"{version_string}+{commit_hash}"

    build_flag = "-D AUTO_VERSION=" + build_version
    print ("Firmware Revision: " + build_version)

    return (build_flag)

env.Append(BUILD_FLAGS=[get_version_build_flag()])