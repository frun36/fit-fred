# `RESET_ERRORS`

## Overview

Clears GBT erros on all boards and sets readiness flag after successful execution.

## WinCC request

WinCC request should be an empty message

## Response to WinCC

If procedure succeded for all boards, the `SUCCESS` message is published.

## Code organization

`RESET_ERRORS` is a simple indefnite mapi class. Requests are intialized within constructor and reused throughout the process. Class holds communication handlers for all boards defined as connected (within `CONNECTED DEVICES` table).