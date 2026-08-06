import os
import subprocess
from importlib.resources import files


def run_x64():
    exe = files("process_bridge") / "bin" / "process-bridge-x64"
    _ = subprocess.run([str(exe)])


def run_x86():
    exe = files("process_bridge") / "bin" / "process-bridge-x86"
    _ = subprocess.run([str(exe)])
