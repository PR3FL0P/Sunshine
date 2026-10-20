# Mouse receipt acknowledgement extension

This experimental branch is based on `v2026.1018.5342`. It advertises
`a=x-ss-input-latency:1` in RTSP DESCRIBE and handles a private mouse envelope
with magic `0x550000F0`. The existing encrypted control channel carries the
original mouse packet and the client's opaque event timestamp.

The parser accepts exactly one relative movement, absolute movement, button-down,
or button-up packet with correct inner and outer sizes. Invalid envelopes are
rejected. Valid envelopes are unwrapped and submitted to the existing input queue.
Sunshine then returns control message `0x55F0` containing the client's timestamp
(LE64) and original mouse packet magic (LE32), through the normal encrypted
control path. Cipher state and replies remain on the control thread.

This acknowledgement confirms receipt and submission to the input queue. It does
not confirm Windows input injection or game processing. The client includes the
return network path in its reported measurement. No clock synchronization is
needed; dividing RTT in half is not claimed as an actual one-way measurement.
Unmodified clients continue to send ordinary input packets.

Use the [matching Moonlight branch](https://github.com/PR3FL0P/moonlight-qt/tree/input-latency).
The client limits movement sampling to 10 times per second and samples every
button transition. The client displays unsupported, waiting, and stale states.

## Validation

`tests/unit/test_input_latency.cpp` adds parser regression tests. After building
Sunshine, run `test_sunshine --gtest_filter=InputLatency.*` (append `.exe` on Windows).
The Moonlight repository also contains `scripts/test-input-latency.py`; it checks
actual client envelope/ACK code against this parser and tests host forwarding/ACK
behavior with mocked transport and encryption boundaries.

The client release build and isolated protocol tests passed on Linux, including
Clang AddressSanitizer and UndefinedBehaviorSanitizer. A full Sunshine Windows
build and end-to-end stream remain unverified. No prebuilt executable is supplied.

## GitHub Actions Windows build

The `Windows Input Latency Build` workflow builds Windows x64 on `windows-2022`
using MSYS2 UCRT64. It uses the same pinned action revisions as the upstream
Windows workflow, builds the checked-out source, runs `InputLatency.*` regression
tests, and uploads a portable ZIP with its SHA-256 checksum and source commit.
It does not require signing credentials or publish a release.

Open the repository's Actions tab, select **Windows Input Latency Build**, choose
**Run workflow** and the `input-latency` branch, and wait for the Windows job.
Download the **Sunshine-Windows-x64-input-latency** artifact from the completed run.
Extract the artifact, then extract its inner Sunshine ZIP before running it.
Artifacts are retained for 30 days. The package is unsigned. Back up the existing
Sunshine configuration and stop the installed Sunshine before starting this build.
A successful build does not replace an end-to-end streaming test on your host.
