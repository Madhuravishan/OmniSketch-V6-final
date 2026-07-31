"""
OmniSketch build.py - cross-platform TypeScript worker build.

Replaces the original Mural build.py which only worked on macOS/Linux.

What this fixes vs. the original:
  1. The original used `cp` and `rm` shell commands which do NOT exist on
     plain Windows. Now uses Python's shutil + os, works everywhere.
  2. The original assumed `npm install` had already been run. Now detects
     missing node_modules/ and runs `npm install --ignore-scripts` once
     automatically (avoids the native-module post-install scripts that
     break on Windows).
  3. The original silently failed if webpack put main.js anywhere other
     than tsc/dist_packed/. Now we explicitly verify the expected output
     exists and fail loudly if it doesn't.
  4. Deletes macOS metadata "ghost" files (._*) from the project tree
     before each build so the C++ compiler doesn't choke on them.
  5. Skips the rebuild entirely if neither the TS sources nor the build
     config have changed since worker.js was last produced. Saves time
     on incremental builds.
"""

import os
import shutil
import subprocess
import sys
import glob

Import("env")

# Paths are relative to the PlatformIO project root.
# Note: __file__ is NOT defined when SCons executes an extra_script,
# so we use os.getcwd() instead. PlatformIO guarantees cwd is the
# project root when build.py runs.
PROJECT_ROOT     = os.getcwd()
TSC_DIR          = os.path.join(PROJECT_ROOT, "tsc")
TSC_DIST         = os.path.join(TSC_DIR, "dist_packed")
TSC_NODE_MODULES = os.path.join(TSC_DIR, "node_modules")
WORKER_OUT_DIR   = os.path.join(PROJECT_ROOT, "data", "www", "worker")
WORKER_OUT_FILE  = os.path.join(WORKER_OUT_DIR, "worker.js")


def fail(message):
    """Print a clear error and abort the PlatformIO build."""
    print("=" * 60)
    print("OmniSketch build.py: FATAL")
    print(message)
    print("=" * 60)
    sys.exit(1)


def run_npm(args, cwd):
    """Run an npm command in a way that works on Windows and Unix.

    On Windows, npm is a .cmd file rather than a binary, so subprocess
    can't find it without shell=True. Using shell=True is fine here
    because we control the argument values.
    """
    cmd = "npm " + " ".join(args)
    print(f"  $ {cmd}    (in {cwd})")
    result = subprocess.run(cmd, cwd=cwd, shell=True)
    if result.returncode != 0:
        fail(f"npm command failed: {cmd}")


def delete_macos_ghost_files(root):
    """Remove ._* metadata files that macOS leaves behind.

    On Mac and Linux these are harmless; on Windows the ESP32 C++ compiler
    tries to compile them as source files and emits confusing errors.
    """
    removed = 0
    for dirpath, _, filenames in os.walk(root):
        for name in filenames:
            if name.startswith("._"):
                try:
                    os.remove(os.path.join(dirpath, name))
                    removed += 1
                except OSError:
                    pass
    if removed:
        print(f"  Removed {removed} macOS ghost file(s) (._*)")


def needs_rebuild():
    """Return True if any TS source / config is newer than worker.js."""
    if not os.path.isfile(WORKER_OUT_FILE):
        return True

    worker_mtime = os.path.getmtime(WORKER_OUT_FILE)

    # Watch all TypeScript sources plus webpack/ts configs.
    watch = []
    watch.extend(glob.glob(os.path.join(TSC_DIR, "src", "**", "*.ts"),  recursive=True))
    watch.extend(glob.glob(os.path.join(TSC_DIR, "src", "**", "*.tsx"), recursive=True))
    for cfg in ("webpack.config.js", "tsconfig.json", "package.json"):
        p = os.path.join(TSC_DIR, cfg)
        if os.path.isfile(p):
            watch.append(p)

    for p in watch:
        if os.path.getmtime(p) > worker_mtime:
            return True
    return False


def main():
    print("OmniSketch build.py: building TypeScript worker")
    print(f"  Project root: {PROJECT_ROOT}")

    # Sanity-check we're in the right place.
    if not os.path.isdir(TSC_DIR):
        fail(f"Expected to find tsc/ at {TSC_DIR}. "
             f"Is build.py in the project root?")

    # 1. Clean macOS ghost files anywhere in the project.
    delete_macos_ghost_files(PROJECT_ROOT)

    # 2. Skip the rebuild if nothing has changed.
    if not needs_rebuild():
        print(f"  worker.js is up to date, skipping rebuild")
        return

    # 3. First-time setup: install npm packages if missing.
    if not os.path.isdir(TSC_NODE_MODULES):
        print("  node_modules/ not found - running first-time npm install")
        print("  (using --ignore-scripts to skip native-module build steps")
        print("   that break on Windows; not needed for the TS pipeline)")
        run_npm(["install", "--ignore-scripts"], cwd=TSC_DIR)

    # 4. Clean previous worker output (the OUTPUT folder, not the source).
    if os.path.isdir(WORKER_OUT_DIR):
        for f in os.listdir(WORKER_OUT_DIR):
            try:
                os.remove(os.path.join(WORKER_OUT_DIR, f))
            except OSError:
                pass
    else:
        os.makedirs(WORKER_OUT_DIR, exist_ok=True)

    # 5. Run webpack via npm.
    run_npm(["run", "build"], cwd=TSC_DIR)

    # 6. Verify webpack actually produced the expected file.
    webpack_output = os.path.join(TSC_DIST, "main.js")
    if not os.path.isfile(webpack_output):
        # Try a deep search to give a helpful diagnostic, then bail.
        candidates = glob.glob(os.path.join(TSC_DIR, "**", "main.js"),
                               recursive=True)
        # Filter out anything inside node_modules (lots of unrelated main.js
        # files live there).
        candidates = [c for c in candidates if "node_modules" not in c]
        msg = (f"webpack did not produce {webpack_output}\n"
               f"  This usually means `npm run build` printed errors above.\n")
        if candidates:
            msg += f"  Files named main.js were found at:\n"
            for c in candidates:
                msg += f"    {c}\n"
            msg += (f"  If one of those is the real build output, "
                    f"update webpack.config.js output.path.")
        fail(msg)

    # 7. Copy to data/www/worker/worker.js (the filename main.js uses).
    print(f"  Copying {webpack_output}")
    print(f"       -> {WORKER_OUT_FILE}")
    shutil.copyfile(webpack_output, WORKER_OUT_FILE)

    size_kb = os.path.getsize(WORKER_OUT_FILE) / 1024.0
    print(f"  worker.js built successfully ({size_kb:.1f} KB)")


main()

