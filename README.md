# Badge Magic for SailfishOS

This repository contains a native SailfishOS application for FOSSASIA LED name badges, built with the SailfishOS SDK, `libsailfishapp`, Sailfish Silica, and BlueZ via `BluezQt`.

For more information on the badges and FOSSASIA, see https://badgemagic.fossasia.org/

## Scope

The SailfishOS version currently covers the core workflow:

- compose text badges
- choose flash, marquee, speed, and animation mode
- save and reload badge presets as JSON
- send the generated payload over BLE to badges exposing service `FEE0` and writable characteristic `FEE1`

## Notes

- The desktop entry requests the `Bluetooth` Sailjail permission because BLE is required for badge transfer.
- The Sailfish backend uses `BluezQt` and the BlueZ D-Bus API.
- Saved badge files are stored under the app data directory and use the Badge Magic JSON payload structure for `messages[0].text`, with an extra optional `rawText` field so the Sailfish editor can reload user-entered text.

## TODO

- Message preview display
- Emojis and custom pictures