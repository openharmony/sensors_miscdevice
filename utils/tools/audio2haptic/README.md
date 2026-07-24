# Audio2Haptic

Audio2Haptic converts one short WAV or MP3 clip into OpenHarmony haptic JSON.
It uses a deterministic, offline DSP pipeline and writes channel `0` only.

## Install

Audio2Haptic requires Python `>=3.12`, `ffmpeg`, and `ffprobe`. Package
metadata installs `numpy`; this repository ships neither host executables nor
audio input files.

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install .
```

## Convert

```bash
audio2haptic convert input.mp3 -o output.json --html output.html
```

On success, the command prints the JSON path and, when requested, the HTML
path. It returns `0`. On admission, compatibility, or output failure, it
prints a stable error code and message to standard error and returns `2`.

The converter never overwrites an existing output path. The optional HTML is
an offline preview of the input waveform and generated events.

See [the supported-input and output contract](docs/SCOPE.md) and
[maintainer validation](docs/VALIDATION.md).
