# Improv Serial dependency

Serial-only subset of jnthas/Improv-WiFi-Library, original commit
`e37c244e0080349e191252acf4bce146eca118e0` (0.0.4).
Source: https://github.com/jnthas/Improv-WiFi-Library/tree/e37c244e0080349e191252acf4bce146eca118e0
Original Jonathas Barbosa MIT license is preserved in LICENSE.
BLE/examples/build metadata are omitted; no BLE implementation is used here.

Vendored under the sketch's src directory so Arduino IDE and PlatformIO both
compile the same patched source without an external library installation.

Changes from that commit:
- Preserve parser reset on a completed RPC, so consecutive requests all work.
- Bound the receive buffer and payload length before writing.
- Validate minimum RPC and Wi-Fi credential field lengths before indexing.
- Initialize callback pointers and device parameters for non-static instances.
- Discard stale partial frames after a 1-second inter-byte gap, with wrap-safe
  time arithmetic, and resynchronize overlapping signature prefixes.
- Make the stream constructor explicit and use equivalent standard checksum sums
  to satisfy the project static-analysis rules.
- Use portable C++17 aggregate initialization and bounded IPv4 formatting.
- Keep setup authorized after a failed Wi-Fi association and report
  ERROR_UNABLE_TO_CONNECT instead of claiming the radio is stopped.
- Offer cooperative association polling with a wrap-safe 30-second timeout,
  replacement/cancellation of pending credentials and no blocking retry loop.
  The Pala wrapper enables it; the legacy blocking API remains available.
- Enforce the SDK SSID/password byte limits before starting the radio.

Pala vendor extension: RPC `0x81` has exactly two payload bytes containing a
lowercase ISO language code (for example `de`). Standard framing and checksums
are unchanged. The firmware callback validates support, prepares the catalog
and persists the selection before accepting. Success clears the error state
and returns one length-prefixed string containing the accepted code. Missing
handlers, unsupported codes, allocation/persistence failures and malformed
payloads cannot change the selected language. The command never starts Wi-Fi.

Native tests compile this exact source with deterministic Stream/Wi-Fi adapters.
They test the existing Improv wire protocol, without accessing real networks.

Vendor RPC `0x82` selects reading-focused menus. Its payload is exactly one ASCII byte (`0` or `1`). A successful reply echoes that value as one RPC string, only after the preference was saved. Other lengths and values are rejected without changing settings or starting Wi-Fi.

During storage recovery the explicit USB allow-list contains only device info (`0x03`), language (`0x81`) and reading menus (`0x82`). These paths use metadata/NVS only. Network state, scans, credentials and unknown commands are rejected before any Wi-Fi API call. Normal provisioning resumes after storage becomes ready.

While a cooperative connection is pending, state queries report PROVISIONING, device metadata remains available and network scans are rejected. A new credentials request replaces the pending attempt. Entering local-only recovery clears pending callback context without any radio call. Success clears the error and saves credentials through the existing callback; timeout returns AUTHORIZED and reports UNABLE_TO_CONNECT.
