# Tests

## Serial / SD card smoke test

### Prerequisites

```powershell
python -m pip install pyserial
```

### Run

```powershell
python .\tests\test_com39.py --port COM39 --baud 115200 --duration 20 `
  --expect SD:PRESENT --expect SD:REMOVED
```

During the test:

1. Start with no card inserted.
2. Insert the card when the test starts.
3. Wait for `SD:PRESENT` / `SD:MOUNT_OK`.
4. Remove the card.
5. Wait for `SD:REMOVED`.

The script exits with code `0` only when all requested patterns were observed.

## Expected firmware messages

```text
SD:PRESENT
SD:REMOVED
SD:MOUNT_OK
SD:MOUNT_FAIL
SD:PART1_OK
SD:PART2_OK
```

## Test configuration

`tests/config/test_config.json` holds the default port, baud rate, duration, and expected patterns. The Python script also accepts command-line overrides.

## Results

Record each run in `docs/TESTS.md` with:

- date,
- firmware version (`0.0x`),
- COM port,
- operation performed (boot, insert, remove, re-insert),
- observed messages,
- pass/fail.
