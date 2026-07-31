#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (c) 2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Build, install, and validate Audio2Haptic outside its source tree."""

from __future__ import annotations

import os
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]


def run(command: list[str], *, cwd: Path, environment: dict[str, str] | None = None) -> None:
    subprocess.run(command, cwd=cwd, env=environment, check=True)


def virtual_environment(directory: Path) -> tuple[Path, dict[str, str]]:
    run([sys.executable, "-m", "venv", str(directory)], cwd=ROOT)
    binary_dir = directory / "bin"
    environment = os.environ.copy()
    environment.pop("AUDIO2HAPTIC_CLI", None)
    environment["PATH"] = f"{binary_dir}{os.pathsep}{environment['PATH']}"
    return binary_dir / "python", environment


def verify_installed_import(python: Path, environment: dict[str, str], directory: Path) -> None:
    command = [
        str(python),
        "-c",
        "import audio2haptic, sys; "
        "from pathlib import Path; "
        "installed = Path(audio2haptic.__file__).resolve(); "
        "expected = Path(sys.argv[1]).resolve(); "
        "raise SystemExit(0 if expected in installed.parents else 1)",
        str(directory),
    ]
    run(command, cwd=directory, environment=environment)


def validate_wheel() -> None:
    with tempfile.TemporaryDirectory(prefix="audio2haptic-wheel-") as temporary_dir:
        temporary_path = Path(temporary_dir)
        wheel_dir = temporary_path / "wheels"
        environment_dir = temporary_path / "venv"
        runner_dir = temporary_path / "runner"
        wheel_dir.mkdir()
        runner_dir.mkdir()
        run([sys.executable, "-m", "pip", "wheel", "--wheel-dir", str(wheel_dir), str(ROOT)], cwd=ROOT)
        python, environment = virtual_environment(environment_dir)
        run(
            [
                str(python), "-m", "pip", "install",
                "--no-index", "--find-links", str(wheel_dir),
                "audio2haptic-openharmony",
            ],
            cwd=runner_dir,
            environment=environment,
        )
        verify_installed_import(python, environment, environment_dir)
        run([str(python), str(ROOT / "scripts" / "smoke_test.py")], cwd=runner_dir, environment=environment)
        run(
            [
                str(python),
                "-m",
                "unittest",
                "discover",
                "-s",
                str(ROOT / "tests"),
                "-t",
                str(ROOT),
            ],
            cwd=runner_dir,
            environment=environment,
        )


def main() -> int:
    try:
        validate_wheel()
    except (OSError, subprocess.CalledProcessError) as exc:
        print(exc, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
