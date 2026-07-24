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

from __future__ import annotations

import argparse
from pathlib import Path
import sys

from ..core.compatibility import CompatibilityError
from ..core.device_profile import ProfileError
from ..core.models import ConversionRequest
from ..core.preprocess import InputError
from .convert import convert
from .convert import PublicationError


EXIT_SUCCESS = 0
EXIT_CONVERSION_FAILED = 1
EXIT_ARGUMENT_ERROR = 2


class _CliArgumentParser(argparse.ArgumentParser):
    def error(self, message: str) -> None:
        self.print_usage(sys.stderr)
        self.exit(EXIT_ARGUMENT_ERROR, f"{self.prog}: error: {message}\n")


def build_parser(*, prog: str = "audio2haptic") -> argparse.ArgumentParser:
    parser = _CliArgumentParser(
        prog=prog,
        description="Convert one short WAV or MP3 file into OpenHarmony haptic JSON.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    convert_parser = subparsers.add_parser("convert", help="Convert one WAV or MP3 file.")
    convert_parser.add_argument("input_path")
    convert_parser.add_argument("-o", "--output", required=True)
    convert_parser.add_argument("--html", help="Write an optional self-contained HTML visualization.")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        result = convert(
            ConversionRequest(
                input_path=Path(args.input_path),
                output_path=Path(args.output),
            ),
            html_path=Path(args.html) if args.html else None,
        )
    except InputError as exc:
        return _emit_error(exc.code, exc.message)
    except CompatibilityError as exc:
        return _emit_error(exc.code, exc.message)
    except ProfileError as exc:
        return _emit_error("invalid_device_profile", str(exc))
    except PublicationError as exc:
        return _emit_error("partial_publish", str(exc))
    except OSError as exc:
        return _emit_error("output_io_error", f"Unable to write output: {exc}")
    print(result.output_path)
    if result.html_path is not None:
        print(result.html_path)
    return EXIT_SUCCESS


def _emit_error(code: str, message: str) -> int:
    print(f"{code}: {message}", file=sys.stderr)
    return EXIT_CONVERSION_FAILED
