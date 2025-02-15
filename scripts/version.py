Import("env")
import datetime
import os

# Get version from git if available
try:
    version = env.GetProjectOption("version")
except:
    try:
        import subprocess
        version = subprocess.check_output(["git", "describe", "--tags"]).strip().decode()
    except:
        version = "1.0.0"

# Get build timestamp
timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

# Create version header
version_header = f"""// Auto-generated version header
#ifndef VERSION_H
#define VERSION_H

#define FIRMWARE_VERSION "{version}"
#define BUILD_TIMESTAMP "{timestamp}"

#endif // VERSION_H
"""

# Write version header
with open("include/version.h", "w") as f:
    f.write(version_header)

# Add version to build flags
env.Append(CPPDEFINES=[
    ("FIRMWARE_VERSION", f'\\"{version}\\"'),
    ("BUILD_TIMESTAMP", f'\\"{timestamp}\\"')
])
