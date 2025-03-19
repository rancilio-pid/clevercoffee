import importlib.metadata

Import("env")

required_pkgs = {'dulwich'}
installed_pkgs = {dist.metadata['Name'] for dist in importlib.metadata.distributions()}
missing_pkgs = required_pkgs - installed_pkgs

if missing_pkgs:
    env.Execute('$PYTHONEXE -m pip install dulwich --global-option="--pure"')

from dulwich.repo import Repo

def get_version_build_flag() -> str:
    r = Repo('.')

    build_version = r.head().decode("utf-8")[0:7]

    build_flag = "-D AUTO_VERSION=\\\"" + build_version + "\\\""
    print ("Firmware Revision: " + build_version)

    return (build_flag)

env.Append(BUILD_FLAGS=[get_version_build_flag()])
