import os
import subprocess
import sys

Import("env")


def _run_wokwi_action(source, target, env):
    env_vars = os.environ.copy()
    env_vars["PIO_CMD"] = f"{sys.executable} -m platformio"
    subprocess.run(
        [sys.executable, "tools/wokwi_flasher_args.py"],
        check=True,
        env=env_vars,
    )


env.AddCustomTarget(
    name="wokwi",
    dependencies=None,
    actions=_run_wokwi_action,
    title="Wokwi",
    description="Build firmware + LittleFS and generate Wokwi flasher args",
)
