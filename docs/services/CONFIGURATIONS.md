# `CONFIGURATIONS`

## Overview
The `CONFIGURATIONS` service is used to fetch and apply configurations, which are sets of parameter name/value pairs stored for each board in the `CONFIGURATIONS` and `CONFIGURATION_PARAMETERS` tables in the FRED DB.

## Service structure
The main service (`FRED/TCM/TCM0/CONFIGURATIONS`) is actually a MAPI group topic. It receives the configuration name from WinCC, and then searches the database for boards to which the configuration should be applied. If the configuration was found, its name gets sent to respective board configuration topics (`FRED/TCM/TCM0/_INTERNAL_CONFIGURATIONS` and `FRED/PM/PMxx/_INTERNAL_CONFIGURATIONS`), and the configuration is applied by them. Responses (formatted like a regular response to WinCC parameter write request) are also published by the `_INTERNAL_CONFIGURATIONS` topics.

## Code organisation

### `Configurations`
This is the parent class of the topic. Within it are contained definitions for all data structures used by the `CONFIGURATIONS` and `_INTERNAL_CONFIGURATIONS` services. It is also responsible for handling the MAPI group request.

### `BoardConfigurations`
This is the base class for PM and TCM `_INTERNAL_CONFIGURATIONS` topics. It contains common logic, such as fetching configuration from the DB and parsing it into the appropriate `ConfigurationInfo` data structure. This structure stores the configuration parsed into a `WinCCRequest`, and values of delay parameters (not present in the request), for which special handling is required in the TCM `_INTERNAL_CONFIGURATIONS` topic.

Static methods of this class are also used for initializing GBT (stored as a configuration in the DB).

### `PmConfigurations`
`_INTERNAL_CONFIGURATIONS` topic for the PM board. Contains the most basic use of `BoardConfigurations` - fetches the configuration, and then sends the parsed request to the electronics.

### `TcmConfigurations`
`_INTERNAL_CONFIGURATIONS` topic for the TCM board. Some additional logic is required with respect to the PM configurations service (based on the Control Server implementation), namely:
- change of `DELAY_A` or `DELAY_C` parameters (handled by the `DelayChange` class). Change of delays/phase is slowed down to 1 unit/ms in the TCM logic, to avoid PLL relock. For larger changes however, the relock will occur nonetheless, causing the `BOARD_STATUS_SYSTEM_RESTARTED` bit to be set. A sleep of appropriate length is then performed, and the `BOARD_STATUS_SYSTEM_RESTARTED` bit is reset.
- additional sleep, performed after the entire operation - waiting for completion of potential re-initialization of the PMs due to change in the channel masks - followed by clearing the `BOARD_STATUS_SYSTEM_RESTARTED` bit again. This part of the service needs some further testing to see if the sleep is performed correctly, and if the readiness bit needs to be reset again.

The topic is therefore implemented as an indefinite MAPI topic, to make implementing this relatively complex logic easier.
