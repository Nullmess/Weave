#!/usr/bin/env python3
"""Provision Flipper's official Sub-GHz region over USB without reflashing firmware."""
import argparse
import logging

from flipper.storage import FlipperStorage
from flipper.utils.cdc import resolve_port
from weave_region import RegionProvisioningError, provision


def main(argv=None):
    parser = argparse.ArgumentParser(description='Restore official Flipper Sub-GHz region provisioning')
    parser.add_argument('-p', '--port', default='auto', help='Flipper USB port (default: auto)')
    parser.add_argument('--dry-run', action='store_true', help='Only fetch and show official server region; do not write')
    args = parser.parse_args(argv)
    logging.basicConfig(level=logging.INFO, format='%(message)s')
    logger = logging.getLogger('WeaveRegion')
    try:
        if args.dry_run:
            from weave_region import fetch_official_region
            country, _ = fetch_official_region()
            print(f'Official Flipper server selected region: {country} (based on PC public IP).')
            print('No changes made. Disconnect VPN/proxy if this is not your physical region.')
            return 0
        port = resolve_port(logger, args.port)
        if not port:
            print('No Flipper found. Connect over USB and close qFlipper before retrying.')
            return 1
        with FlipperStorage(port, chunk_size=256, cli_timeout=30) as storage:
            # Deliberately replaces a corrupt or outdated region when explicitly requested.
            country, changed = provision(storage, logger=logger, force=True)
            if changed:
                print(f'Official Sub-GHz region {country} written and verified; rebooting Flipper to activate.')
                storage.send('power reboot\r')
            else:
                print('Sub-GHz region is already present; no reboot needed.')
        return 0
    except (RegionProvisioningError, OSError, ValueError, RuntimeError) as error:
        logger.error('Cannot provision Sub-GHz region: %s', error)
        logger.error('Alternative: qFlipper > Install from file > Weave firmware .tgz')
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
