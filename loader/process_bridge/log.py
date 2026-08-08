from __future__ import annotations

import logging
import os

_PACKAGE_LOGGER_NAME = "process_bridge"
_ENV_VAR = "PROCESS_BRIDGE_LOG_LEVEL"
_configured = False


def _configure_once() -> None:
    global _configured
    if _configured:
        return
    _configured = True

    package_logger = logging.getLogger(_PACKAGE_LOGGER_NAME)

    handler = logging.StreamHandler()
    handler.setFormatter(
        logging.Formatter(
            fmt="%(asctime)s [%(levelname)-7s] %(name)s: %(message)s",
            datefmt="%H:%M:%S",
        )
    )
    package_logger.addHandler(handler)

    level_name = os.environ.get(_ENV_VAR, "INFO").upper()
    level = logging.getLevelName(level_name)
    if not isinstance(level, int):
        package_logger.warning(
            "unknown %s=%r, expected DEBUG/INFO/WARNING/ERROR, defaulting to INFO",
            _ENV_VAR,
            level_name,
        )
        level = logging.INFO

    package_logger.setLevel(level)

    package_logger.propagate = False


def get_logger(module_name: str) -> logging.Logger:
    _configure_once()
    return logging.getLogger(module_name)
