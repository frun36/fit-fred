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
