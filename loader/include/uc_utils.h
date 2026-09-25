#pragma once

#define PB_UC_CHECK(expr, msg)                       \
  do {                                               \
    uc_err _err = (expr);                            \
    if (_err != UC_ERR_OK) {                         \
      LOG_FATAL("%s: %s", (msg), uc_strerror(_err)); \
      return -1;                                     \
    }                                                \
  } while (0)
