#!/usr/bin/env python3
import subprocess
import sys
import os

REPO_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_DIR = os.path.join(REPO_DIR, "build")

def run(cmd, check=True):
    result = subprocess.run(cmd, capture_output=True, text=True, cwd=REPO_DIR)
    if check and result.returncode != 0:
        print(f"Error: {result.stderr.strip()}")
        sys.exit(1)
    return result

def get_existing_releases():
    result = run(["gh", "release", "list", "--json", "tagName", "-q", ".[].tagName"])
    return result.stdout.strip().splitlines()

def main():
    installers = [f for f in os.listdir(BUILD_DIR) if f.endswith((".deb", ".exe"))] if os.path.isdir(BUILD_DIR) else []
    if installers:
        print("Available installer files:")
        for f in installers:
            print(f"  - {f}")

    installer_name = input("Enter installer filename (.deb or .exe): ").strip()
    if not installer_name:
        print("No filename entered. Aborting.")
        sys.exit(1)

    installer_path = os.path.join(BUILD_DIR, installer_name)
    if not os.path.isfile(installer_path):
        print(f"Error: {installer_path} not found. Run build-package.sh first.")
        sys.exit(1)

    existing = get_existing_releases()
    if existing:
        print("Existing releases:", ", ".join(existing))
    else:
        print("No existing releases found.")

    version = input("Enter release version (e.g. v0.1): ").strip()
    if not version:
        print("No version entered. Aborting.")
        sys.exit(1)

    notes = input("Enter release notes (or press Enter for default): ").strip()
    if not notes:
        exe_name = installer_name.split("-")[0]
        notes = f"{exe_name} {version}"

    if version in existing:
        print(f"Release {version} already exists. Replacing...")
        run(["gh", "release", "delete", version, "--yes", "--cleanup-tag"])

    print(f"Creating release {version}...")
    run(["gh", "release", "create", version, installer_path, "--title", version, "--notes", notes])
    print(f"Release {version} published successfully.")

if __name__ == "__main__":
    main()
