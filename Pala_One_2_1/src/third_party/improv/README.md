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
- Use portable C++17 aggregate initialization and bounded IPv4 formatting. No private/vendor commands are introduced.

Native tests compile this exact source with deterministic Stream/Wi-Fi adapters.
They test the existing Improv wire protocol, without accessing real networks.
