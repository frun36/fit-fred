# `RESET_SYSTEM`

## Overview

`RESET SYSTEM` is a crucial subdetector-wide routine in the FIT DCS workflow. `RESET SYSTEM` restarts the FEE electronics, identifies active PMs, and corrects board IDs and/or trigger signatures.

There are four major phases:

### I. Applying the reset system command
1. Switch off GBT errors
2. Apply reset system command (and optionaly force local clock)
3. Sleep for 2000ms
4. Set readines bit to 1
5. Switch on GBT errors

### II. Applying trigger signatures from the FEE_SETTINGS table
1. Apply all five trigger signatures

### III. Testing the PMs' SPI connection and updating the PM SPI mask
1. Switch on given PM' SPI mask bit 
2. Send read request to PM (read high voltage)
3. Verify if communication has succeded - if not, switch off PM' bit
4. Repeat steps 1-3 for all PMs 

### IV. Veryfing all board 
1. Verify TCM board ID and subdetector ID - if they are not in agreement with `FEE_SETTINGS`, substitute them with the appropriate ones and apply default GBT configuration

2. Verify PMs board ID and subdetector ID - if they are not in agreement with `FEE_SETTINGS`, substitute them with the appropriate ones and apply default GBT configuration

## WinCC Request

Empty string or `FORCE LOCAL CLOCK` to enforce using local clock. There is also `ENFORCE_DEFAULT_GBT_CONFIG`, but it is advised to use it only in the test phase.

## Response to WinCC

If procedure succeded, `SUCCESS` is sent back.

## Error handling

In case of failure at any stage, the error service is used to publish detailed error message.