# Audio2Haptic Contract

Audio2Haptic is a host-only developer utility, not a device build target. It
converts one short input clip with deterministic DSP; conversion has no model
download, network request, or device playback step.

## Input

- WAV and MP3 are the only accepted formats.
- Input duration must not exceed 10 seconds.
- Mono input is preserved. Stereo input is deterministically downmixed to
  mono. Inputs with more than two channels are rejected.
- `ffmpeg` and `ffprobe` are explicit host prerequisites. The converter limits
  their input to the named local file and does not read their standard input.

## Output

The public command is:

```bash
audio2haptic convert INPUT -o OUTPUT [--html HTML]
```

- `OUTPUT` is one OpenHarmony haptic JSON payload with `ChannelNumber: 1` and
  channel index `0`.
- The emitted payload stays within the local decoder compatibility envelope:
  at most 128 events, continuous events no longer than 5000 ms, curves with
  4 through 16 points, and an encoded JSON size no greater than 64 KiB.
- Continuous events are split before the decoder's fixed 16-event playback
  handoffs so that a later playback packet cannot truncate an earlier event.
- The converter will not overwrite either requested output. If publication
  cannot be rolled back completely, it reports `partial_publish` and names
  the affected path.
- `--html` writes one self-contained, offline visualization.

## Exclusions

This tool does not process clips longer than 10 seconds, accept other input
formats, select a nonzero channel, or initialize external models. `numpy` is
resolved through package metadata; host executables and user audio remain
outside the repository.
