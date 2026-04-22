#!/usr/bin/env python3
import subprocess
import sys
import os

DEB_PATH = "build/orvocapp-0.1-Linux.deb"

def run(cmd, check=True):
    result = subprocess.run(cmd, capture_output=True, text=True)
    if check and result.returncode != 0:
        print(f"Error: {result.stderr.strip()}")
        sys.exit(1)
    return result

def get_existing_releases():
    result = run(["gh", "release", "list", "--json", "tagName", "-q", ".[].tagName"])
    return result.stdout.strip().splitlines()

def main():
    if not os.path.isfile(DEB_PATH):
        print(f"Error: {DEB_PATH} not found. Run build-package.sh first.")
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
        notes = f"orVocApp {version}"

    if version in existing:
        print(f"Release {version} already exists. Replacing...")
        run(["gh", "release", "delete", version, "--yes", "--cleanup-tag"])

    print(f"Creating release {version}...")
    run(["gh", "release", "create", version, DEB_PATH, "--title", version, "--notes", notes])
    print(f"Release {version} published successfully.")

if __name__ == "__main__":
    main()
