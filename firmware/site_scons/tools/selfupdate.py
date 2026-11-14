#!/usr/bin/env python3

import logging
import os
import pathlib

from flipper.app import App
from flipper.storage import FlipperStorage, FlipperStorageOperations
from flipper.utils.cdc import resolve_port
from weave_region import RegionProvisioningError, provision


class Main(App):
    def init(self):
        self.parser.add_argument("-p", "--port", help="CDC Port", default="auto")
        self.parser.add_argument("--chunk-size", type=int, default=1024,
                                 choices=(256, 512, 1024, 2048, 4096, 8192),
                                 help="USB CDC chunk size in bytes (default 1024)")
        self.parser.add_argument("--chunk-timeout", type=int, default=45,
                                 help="Timeout for each transfer chunk in seconds")

        self.parser.add_argument("manifest_path", help="Manifest path")
        self.parser.add_argument(
            "--pkg_dir_name", help="Update dir name", default=None, required=False
        )
        self.parser.set_defaults(func=self.install)


        self.logger = logging.getLogger()

    def install(self):
        if not (port := resolve_port(self.logger, self.args.port)):
            return 1

        if not os.path.isfile(self.args.manifest_path):
            self.logger.error("Error: manifest not found")
            return 2

        manifest_path = pathlib.Path(os.path.abspath(self.args.manifest_path))
        manifest_name, pkg_name = manifest_path.parts[-1], manifest_path.parts[-2]

        pkg_dir_name = self.args.pkg_dir_name or pkg_name
        update_root = "/ext/update"
        flipper_update_path = f"{update_root}/{pkg_dir_name}"

        self.logger.info(f'Installing "{pkg_name}" from {flipper_update_path}')

        try:
            if not 10 <= self.args.chunk_timeout <= 180:
                self.logger.error("--chunk-timeout must be between 10 and 180 seconds")
                return 5
            self.logger.info(
                "USB transfer settings: %d-byte chunks, %ds stall timeout",
                self.args.chunk_size, self.args.chunk_timeout,
            )
            with FlipperStorage(
                port, chunk_size=self.args.chunk_size,
                cli_timeout=self.args.chunk_timeout,
            ) as storage:
                # The custom USB updater does not have qFlipper's built-in
                # regional provisioning. Restore the official /int/.region_data
                # before installing the new firmware (activated on reboot).
                try:
                    country, changed = provision(storage, self.logger)
                    if changed:
                        self.logger.info('Sub-GHz regulatory region %s will be active after update', country)
                except RegionProvisioningError as error:
                    self.logger.warning('Sub-GHz region provisioning unavailable: %s', error)
                    self.logger.warning('After flashing, run: python3 fbt.py region')
                storage_ops = FlipperStorageOperations(storage)
                storage_ops.mkpath(update_root)
                storage_ops.mkpath(flipper_update_path)
                storage_ops.recursive_send(
                    flipper_update_path, manifest_path.parents[0]
                )

                storage.send_and_wait_eol(
                    f"update install {flipper_update_path}/{manifest_name}\r"
                )
                result = storage.read.until(storage.CLI_EOL, timeout=180)
                if b"Verifying" not in result:
                    self.logger.error(f"Unexpected response: {result.decode('ascii')}")
                    return 3
                result = storage.read.until(storage.CLI_EOL, timeout=180)
                if not result.startswith(b"OK"):
                    self.logger.error(result.decode("ascii"))
                    return 4
                return 0
        except Exception as e:
            self.logger.error("USB CDC transfer failed: %s", e)
            self.logger.error("Check the Flipper SD card and USB cable. No update success was confirmed.")
            return 5


if __name__ == "__main__":
    Main()()
