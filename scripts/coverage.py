Import("env")
import os
import subprocess

def generate_coverage(source, target, env):
    # Get build directory
    build_dir = env.subst("$BUILD_DIR")
    
    # Create coverage directory
    coverage_dir = os.path.join(build_dir, "coverage")
    os.makedirs(coverage_dir, exist_ok=True)
    
    # Generate coverage report
    subprocess.run([
        "gcovr",
        "--html-details",
        f"--html={os.path.join(coverage_dir, 'coverage.html')}",
        "--exclude-throw-branches",
        "--exclude-unreachable-branches",
        "--print-summary",
        "--root", ".",
        "--object-directory", build_dir,
        "-j", "4",  # Use 4 threads
        "--exclude", "test/.*",  # Exclude test files
        "--exclude", "lib/.*",   # Exclude library files
    ])
    
    # Generate text report
    with open(os.path.join(coverage_dir, "coverage.txt"), "w") as f:
        subprocess.run([
            "gcovr",
            "--txt",
            "--exclude-throw-branches",
            "--exclude-unreachable-branches",
            "--print-summary",
            "--root", ".",
            "--object-directory", build_dir,
            "-j", "4",
            "--exclude", "test/.*",
            "--exclude", "lib/.*",
        ], stdout=f)
    
    # Print summary
    print("\nCoverage Report Generated:")
    print(f"HTML Report: {os.path.join(coverage_dir, 'coverage.html')}")
    print(f"Text Report: {os.path.join(coverage_dir, 'coverage.txt')}")
    
    # Check coverage thresholds
    result = subprocess.run([
        "gcovr",
        "--fail-under-line", "80",  # Require 80% line coverage
        "--fail-under-branch", "70", # Require 70% branch coverage
        "--root", ".",
        "--object-directory", build_dir,
        "--exclude", "test/.*",
        "--exclude", "lib/.*",
    ])
    
    if result.returncode != 0:
        print("\nWARNING: Coverage below thresholds!")
        print("Required: 80% line coverage, 70% branch coverage")

# Register callback
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", generate_coverage)
