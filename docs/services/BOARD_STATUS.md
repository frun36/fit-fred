# `BOARD STATUS`

## Overview

At one-second intervals, the service updates electronic board status. Status is defined as a set of parameters identified in the database by the `SYNC` refresh type. Along status update, `BOARD STATUS` calculates GBT rates and, in case of GBT error, error report is saved to file.

## WinCC Request

`BOARD STATUS` is a self-looping service and should not be requested by WinCC.

## Response to WinCC

`BOARD_PARAMETERS` sends values of all `SYNC` parameters, GBT rates and/or GBT errors flag.

```
[PARAMETER_A],[VALUE]
[PARAMETER_B],[VALUE]
...
```

## Error handling

- In case of failure in WinCC request parsing, the error is published immeadiately.

- In case of failure in ALF response parsing, the error message is published along with the part of data that was successfuly retrieved from message.


## Code organisation

`BoardStatus` is implemented as a class inheriting from the IndefiniteMapi interface.

### Parent classes
- `BasicFitIndefiniteMapi` - provides method for FIFO read, which is neccessary to fetch information about GBT errors
- `GbtRateMonitor` - provides method for GBT rate calculation

### Members
- `BoardCommunicationHandler m_boardHandler` - handles communication during status update, usage is optimized by calling `processMessageFromWinCC` once in the constructor to prepare request data and request object (`m_request`), then reusing it throughout the time of execution, this impose limitation that this handler cannot be used to process any other request
- `BoardCommunicationHandler m_gbtFifoHandler` -  handles communication during GBT error FIFO read
- `SwtSequence m_request` - status update request, created at the initialization through `m_handler`
- `std::shared_ptr<gbt::GbtErrorType> m_gbtError` - pointer to GBT error object, created if error was detected (`checkGbtError`) and destroyed after saving report to file

### Methods
- `void updateEnvironment()` -  executed only in TCM service, updates `Environement` variables according to the recent values of electronic parameters
- `BoardCommunicationHandler::ParsedResponse checkGbtErrors()` - if GBT error flag is on, then reads GBT error FIFO and constructs GBT error object (`m_gbtError`)

