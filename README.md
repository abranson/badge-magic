# Badge Magic for SailfishOS

This repository contains a native SailfishOS application for FOSSASIA LED name badges, built with the SailfishOS SDK, `libsailfishapp`, Sailfish Silica, and the BlueZ D-Bus API.

For more information on the badges and FOSSASIA, see https://badgemagic.fossasia.org/

## Scope

The SailfishOS version currently covers the core workflow:

- compose text badges
- choose flash, marquee, speed, and animation mode
- save and reload badge presets as JSON
- batch-send up to 8 saved badges as hardware-switchable message slots
- send the generated payload over BLE to badges exposing service `FEE0` and writable characteristic `FEE1`

## Notes

- The desktop entry requests the `Bluetooth` Sailjail permission because BLE is required for badge transfer.
- The Sailfish backend uses asynchronous BlueZ D-Bus calls and sends one chunk at a time, after connecting and resolving the badge services.
- Saved badge files are stored under the app data directory and use the Badge Magic JSON payload structure for `messages[0].text`, with optional `rawText` and `name` fields for editing and display. New presets have unique filenames; saving the same display name updates that preset. Legacy filename-based presets remain supported.
- Saved badges and app preferences support backup and restore through [My Backup](https://openrepos.net/content/slava/my-backup). Select Badge Magic in My Backup to include the saved presets and configuration (currently the preview colour) in SailfishOS backups.
- Message-slot transfers follow the upstream Badge Magic packet format: up to 8 saved presets are packed into one BLE payload, and the badge button cycles through them.

## TODO

- Message preview display
- Emojis and custom pictures

## Tests

See [the host regression test instructions](tests/README.md) for isolated Bluetooth, packet, preset and preview checks.
