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

## Code organisation

### Parent class
- `BasicFitIndefiniteMapi` - provides a comprehensive method for sending requests to ALF, handling invalid request and parsing reponse

### Static members

- `constexpr std::string_view EnforceDefGbtConfig` - contains request to enforce default gbt application
- `constexpr std::string_view ForceLocalClock` - contains request to force local clock usage

### Members
- `bool m_enforceDefGbtConfig` - set to true if WinCC request contains `EnforceDefGbtConfig` message
- `bool m_forceLocalClock` - set to true if WinCC request contains `ForceLocalClock` message
- `std::chrono::milliseconds m_sleepAfterReset` - duration of sleep after applying the reset command
- `std::vector<BoardCommunicationHandler> m_PMs` - objects to handle communication with PMs
- `BoardCommunicationHandler m_TCM` - object ot handle communication with TCM

### Methods
- `std::string seqSwitchGBTErrorReports(bool)` - returns request to switch on/off GBT error report generation

- `std::string seqSetResetSystem()` - returns reset system request

- `std::string seqSetResetFinished()` - returns request setting readiness flag

- `std::string seqSetBoardId(std::shared_ptr<Board> board)` - returns request to set board ID based on the `Environment` variables

- `std::string seqSetSystemId()` - returns request to set system ID based on the `Environment` variables

- `std::string seqMaskPMLink(uint32_t idx, bool mask)` - returns request to switch on/off PM in SPI mask

- `BoardCommunicationHandler::ParsedResponse applyResetFEE()` - proceed with the FEE reset. If the procedure succeeds, it returns an empty response; otherwise, the last response from ALF is returned

- `BoardCommunicationHandler::ParsedResponse testPMLinks()` - updates SPI mask, sends request to each possibly connected PM (based on the data from the `CONNECTED DEVICES` table fetched before FRED execution). If request fails, PM is disabled.

- `BoardCommunicationHandler::ParsedResponse applyGbtConfiguration()` - verifies board ID and system ID on each board. If either of IDs is in disagreement with the `Environment`, IDs are corrected and the default gbt configuration is applied. Default GBT configuration may also be enforced if `m_enforceDefGbtConfig` is true.

- `BoardCommunicationHandler::ParsedResponse applyGbtConfigurationToBoard(BoardCommunicationHandler& boardHandler)` - applies default GBT configuration to board 

- `BoardCommunicationHandler::ParsedResponse applyTriggersSign()` - updates trigger signatures (signatures are defined within `Environment`).