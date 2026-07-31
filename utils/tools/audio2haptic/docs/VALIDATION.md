# Validation

Run the following commands from this tool directory after `pip install .`.

```bash
python3 -m unittest discover -s tests -t . -p 'test_*.py'
python3 scripts/validate_installed_wheel.py
```

The `unittest` command tests the source tree. `validate_installed_wheel.py`
builds and installs the package in a temporary virtual environment, then
exercises its WAV, MP3, HTML, and unit-test paths. Test audio is generated at
runtime.

Generated signals cover silence and noise rejection, transient and continuous
events, mixed beds and accents, stereo downmix, dense event planning, and
packet boundaries.

Install `pylint` and `coverage` to run the quality gates:

```bash
python3 -m pylint audio2haptic scripts tests
python3 -m coverage run -m unittest discover -s tests -t . -p 'test_*.py'
python3 -m coverage report --include='audio2haptic/*' --fail-under=95
```
