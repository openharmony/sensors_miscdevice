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

import os
from dataclasses import dataclass
from pathlib import Path
import tempfile

from ..core.compatibility import validate_openharmony_payload
from ..core.conversion import prepare_dsp_conversion
from ..core.models import ConversionRequest
from ..core.preprocess import InputError
from ..core.renderer import render_openharmony
from ..core.serialization import encode_json
from .visualize import build_visualization_html


@dataclass(frozen=True)
class ConversionResult:
    output_path: Path
    html_path: Path | None
    event_count: int


@dataclass(frozen=True)
class CleanupFailure:
    path: Path
    error: OSError


class PublicationError(OSError):
    def __init__(self, publish_error: OSError, cleanup_failures: list[CleanupFailure]):
        self.publish_error = publish_error
        self.cleanup_failures = tuple(cleanup_failures)
        cleanup_details = "; ".join(_cleanup_failure_message(item) for item in cleanup_failures)
        super().__init__(f"{_os_error_message(publish_error)}; cleanup failed: {cleanup_details}")


def convert(request: ConversionRequest, *, html_path: Path | None = None) -> ConversionResult:
    _validate_destinations(request, html_path)
    prepared = prepare_dsp_conversion(request)
    payload = render_openharmony(prepared.events)
    json_bytes = encode_json(payload)
    validate_openharmony_payload(payload, json_size_bytes=len(json_bytes))
    files = {request.output_path: json_bytes}
    if html_path is not None:
        files[html_path] = build_visualization_html(prepared.audio, payload).encode("utf-8")
    _publish_files(files)
    return ConversionResult(
        output_path=request.output_path,
        html_path=html_path,
        event_count=len(prepared.events),
    )


def _validate_destinations(request: ConversionRequest, html_path: Path | None) -> None:
    destinations = [request.output_path]
    if html_path is not None:
        destinations.append(html_path)
    resolved = [path.resolve() for path in destinations]
    if request.input_path.resolve() in resolved:
        raise InputError("output_path", "input_output_conflict", "Output path must differ from input audio.")
    if len(set(resolved)) != len(resolved):
        raise InputError("output_path", "output_path_conflict", "JSON and HTML paths must differ.")
    if any(path.exists() for path in destinations):
        raise InputError("output_path", "output_already_exists", "Output path already exists.")


def _publish_files(files: dict[Path, bytes]) -> None:
    parents = {path.parent for path in files}
    for parent in parents:
        parent.mkdir(parents=True, exist_ok=True)
    staged = _stage_files(files)
    try:
        _link_staged_files(staged)
    except OSError as exc:
        cleanup_failures = _remove_staged_files(staged)
        if cleanup_failures:
            raise PublicationError(exc, cleanup_failures) from exc
        raise
    cleanup_failures = _remove_staged_files(staged)
    if cleanup_failures:
        cleanup_error = OSError("Published output but could not remove temporary files")
        raise PublicationError(cleanup_error, cleanup_failures)


def _stage_files(files: dict[Path, bytes]) -> list[tuple[Path, Path]]:
    staged: list[tuple[Path, Path]] = []
    try:
        for target, content in files.items():
            staged.append(_stage_file(target, content))
    except OSError as exc:
        cleanup_failures = _remove_staged_files(staged)
        if cleanup_failures:
            raise PublicationError(exc, cleanup_failures) from exc
        raise
    return staged


def _stage_file(target: Path, content: bytes) -> tuple[Path, Path]:
    output = tempfile.NamedTemporaryFile(
        mode="wb",
        prefix=f".{target.stem}.",
        dir=target.parent,
        delete=False,
    )
    temporary_path = Path(output.name)
    try:
        with output:
            output.write(content)
    except OSError as exc:
        cleanup_failure = _remove_file(temporary_path)
        if cleanup_failure is not None:
            raise PublicationError(exc, [cleanup_failure]) from exc
        raise
    return temporary_path, target


def _remove_staged_files(staged: list[tuple[Path, Path]]) -> list[CleanupFailure]:
    failures = []
    for temporary_path, _ in staged:
        failure = _remove_file(temporary_path)
        if failure is not None:
            failures.append(failure)
    return failures


def _remove_file(path: Path) -> CleanupFailure | None:
    try:
        path.unlink()
    except FileNotFoundError:
        # Cleanup is idempotent when another path has already removed the file.
        return None
    except OSError as exc:
        return CleanupFailure(path, exc)
    return None


def _link_staged_files(staged: list[tuple[Path, Path]]) -> None:
    published: list[tuple[Path, tuple[int, int]]] = []
    try:
        for temporary_path, target in staged:
            os.link(temporary_path, target)
            published.append((target, _file_identity(temporary_path)))
    except OSError as exc:
        cleanup_failures = []
        for target, identity in published:
            failure = _unlink_if_owned(target, identity)
            if failure is not None:
                cleanup_failures.append(failure)
        if cleanup_failures:
            raise PublicationError(exc, cleanup_failures) from exc
        raise


def _file_identity(path: Path) -> tuple[int, int]:
    status = os.lstat(path)
    return status.st_dev, status.st_ino


def _unlink_if_owned(target: Path, identity: tuple[int, int]) -> CleanupFailure | None:
    try:
        # The inode check prevents rollback from deleting a concurrent replacement.
        if _file_identity(target) == identity:
            target.unlink()
    except FileNotFoundError:
        # A missing target already satisfies the rollback operation.
        return None
    except OSError as exc:
        return CleanupFailure(target, exc)
    return None


def _cleanup_failure_message(failure: CleanupFailure) -> str:
    return f"{failure.path}: {_os_error_message(failure.error)}"


def _os_error_message(error: OSError) -> str:
    if error.errno is None:
        return str(error)
    reason = error.strerror or str(error)
    return f"[Errno {error.errno}] {reason}"
