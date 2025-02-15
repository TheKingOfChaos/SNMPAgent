Import("env")
import os

def analyze_size(source, target, env):
    # Get firmware size
    firmware_path = str(target[0])
    size = os.path.getsize(firmware_path)
    
    # Calculate size percentages
    flash_size = 2 * 1024 * 1024  # 2MB
    ram_size = 264 * 1024         # 264KB
    
    flash_percent = (size / flash_size) * 100
    
    # Get RAM usage from map file
    map_file = firmware_path.replace(".elf", ".map")
    ram_used = 0
    
    if os.path.exists(map_file):
        with open(map_file, "r") as f:
            for line in f:
                if ".data" in line or ".bss" in line:
                    try:
                        ram_used += int(line.split()[1], 16)
                    except:
                        pass
    
    ram_percent = (ram_used / ram_size) * 100
    
    # Print analysis
    print("\nFirmware Size Analysis:")
    print("-----------------------")
    print(f"Flash Usage: {size:,} bytes ({flash_percent:.1f}% of {flash_size:,} bytes)")
    print(f"RAM Usage:  {ram_used:,} bytes ({ram_percent:.1f}% of {ram_size:,} bytes)")
    
    # Check limits
    if flash_percent > 70:
        print("\nWARNING: Flash usage exceeds 70% limit!")
    if ram_percent > 50:
        print("\nWARNING: RAM usage exceeds 50% limit!")
    
    print("\nMemory Map:")
    print("-----------")
    os.system(f"arm-none-eabi-nm --print-size --size-sort --radix=d {firmware_path} | tail -n 20")

# Register callback
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", analyze_size)
