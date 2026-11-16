# Shared firmware release integration

This branch integrates the completed local reader behavior into the public
firmware before promoting a shared release to `main`. Public release text,
commit messages, pull requests, instructions and flasher text are English.
The eleven selectable device languages remain available.

## Source boundaries

- Public integration base: `71e3f84050657a1c81b8e37198f23a0e09ce2e46`.
- Preserved local source: `55516072191c50a1b4df0d84354ee87d8a053298`.
- The local source is a reference, not a wholesale replacement for public dev.
  Existing public apps, settings, custom menu order, protocol hardening,
  complete catalogs and community attribution must remain compatible.
- Existing OTA partitions and canonical reading positions remain intact.

## Release work

1. Integrate wake/input behavior, reading performance, cache/large-book handling
   and the completed local navigation/layout behavior. Provide reading-focused
   navigation as a selectable view without deleting installed apps or settings.
2. Preserve public persistence/protocol formats, add explicit compatibility
   handling where the local source uses another format, and retain failure
   reporting for recovery, uploads and reset operations.
3. Build both board revisions with all initial-language variants; run the
   relevant host checks and autonomous V1.2 device checks. Hardware evidence
   must not be claimed for V1.1. Do not request external reviews or tests.
4. Promote the verified shared source to main, publish a versioned release with
   board-specific artifacts and checksums, and retain a rollback release.
5. Update the online flasher and English instructions to that exact release.
   Runtime language selection remains separate from board revision selection.

## Integrated so far

Wake capture now starts before serial, battery, filesystem and display work.
The 200 ms serial delay remains on cold boot only. V1.2 replaces the sleep image
with the requested full frame directly, while V1.1 retains its panel setup.
Unlocked wake suppresses only the boot press and preserves subsequent gestures;
locked wake retains its unlock gesture. Separate queued clicks are dispatched
in order even when a display refresh delayed polling. Existing ClickTimings
owners, settings and public persistence keys are retained.

The real input frontend and extracted actual sketch setup/restore branches are
covered by native checks for both boards, including cold/warm boot, locked and
unlocked states, library/reader, retained first input and the failed-mount guard.
The complete V1.2 image builds within the existing OTA slot. This checkpoint is
not a release and does not represent a new device flash.

The current integration also stages converged page text in a bounded 4 KiB buffer, with a complete direct-render fallback when it fills. Real-font tests compare staged and direct pixels. Reading-focused menus are opt-in through the web and typed companion settings; the original custom order and apps remain intact. Firmware/help, statistics and update layouts use fixed menu typography and bounded cards. Static browser stylesheets are served as deterministic gzip assets, preserving their decoded bytes and reducing embedded payload size.
