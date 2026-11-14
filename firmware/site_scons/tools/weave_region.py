"""Official Flipper Sub-GHz region provisioning for Weave USB-CDC installs.

The region is selected by Flipper's *official* server from the host network
location, never guessed from the language, SD card, or firmware hardware id.
No out-of-region radio settings are fabricated or installed.
"""

import hashlib
import json
import logging
import re
import tempfile
import urllib.request
from pathlib import Path

OFFICIAL_REGION_URL = 'https://update.flipperzero.one/regions/api/v0/bundle'
REGION_PATH = '/int/.region_data'
TEMP_REGION_PATH = '/int/.region_data.weave-part'
BACKUP_REGION_PATH = '/int/.region_data.weave-backup'
MAX_BUNDLE_BYTES = 256 * 1024


class RegionProvisioningError(RuntimeError):
    pass


def _varint(value):
    if value < 0:
        value &= (1 << 64) - 1  # protobuf int32 negative: sign-extended 64-bit varint
    result = bytearray()
    while value > 127:
        result.append((value & 127) | 128)
        value >>= 7
    result.append(value)
    return bytes(result)


def _field_varint(number, value):
    return _varint(number << 3) + _varint(value)


def _field_bytes(number, value):
    return _varint((number << 3) | 2) + _varint(len(value)) + value


def encode_official_region(bundle):
    """Convert official bands and geo-selected country to PB.Region (flipper.proto)."""
    try:
        info = bundle['success']
        country = info['country']
        bands_by_name = info['bands']
        names = info['countries'][country]
    except (KeyError, TypeError) as exc:
        raise RegionProvisioningError('Official region bundle is incomplete') from exc

    if not isinstance(country, str) or not re.fullmatch('[A-Z]{2}', country):
        raise RegionProvisioningError('Official server supplied an invalid country')
    if not isinstance(names, list) or not 1 <= len(names) <= 32:
        raise RegionProvisioningError(f'No official Sub-GHz bands for {country}')

    result = bytearray(_field_bytes(1, country.encode('ascii')))
    for name in names:
        try:
            band = bands_by_name[name]
            start, end = band['start'], band['end']
            power, duty = band['max_power'], band['duty_cycle']
        except (KeyError, TypeError) as exc:
            raise RegionProvisioningError('Official region bundle has an invalid band') from exc
        if (not all(type(x) is int for x in (start, end, power, duty)) or
                not (1 <= start < end <= 1_000_000_000) or
                not (-128 <= power <= 127) or not (0 <= duty <= 100)):
            raise RegionProvisioningError(f'Invalid official Sub-GHz band {name}')
        payload = b''.join((
            _field_varint(1, start), _field_varint(2, end),
            _field_varint(3, power), _field_varint(4, duty),
        ))
        result.extend(_field_bytes(2, payload))
    if len(result) > 4096:
        raise RegionProvisioningError('Official region payload exceeds size limit')
    return country, bytes(result)


def fetch_official_region(timeout=12):
    request = urllib.request.Request(
        OFFICIAL_REGION_URL,
        headers={'Accept': 'application/json', 'User-Agent': 'Weave/1.0.0 FlipperRegionProvisioner'},
    )
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            # Only trust the official HTTPS endpoint, including after redirects.
            if not response.geturl().startswith('https://update.flipperzero.one/regions/'):
                raise RegionProvisioningError('Official region endpoint redirected elsewhere')
            raw = response.read(MAX_BUNDLE_BYTES + 1)
    except (OSError, ValueError) as exc:
        raise RegionProvisioningError(f'Cannot contact the official Flipper region server: {exc}') from exc
    if len(raw) > MAX_BUNDLE_BYTES:
        raise RegionProvisioningError('Official region response too large')
    try:
        return encode_official_region(json.loads(raw))
    except (ValueError, UnicodeError) as exc:
        raise RegionProvisioningError('Official region endpoint did not return usable JSON') from exc


def _storage_command(storage, command, path):
    storage.send_and_wait_eol(command + '\r')
    result = storage.read.until(storage.CLI_EOL)
    storage.read.until(storage.CLI_PROMPT)
    storage._check_no_error(result, path)
    return result


def provision_storage(storage, country, content, force=False):
    """Upload to internal storage, verify bytes, and swap with rollback on error."""
    if storage.exist_file(REGION_PATH) and not force and storage.size(REGION_PATH) > 0:
        return False

    expected_md5 = hashlib.md5(content).hexdigest()
    had_previous = False
    try:
        with tempfile.TemporaryDirectory(prefix='weave-region-') as directory:
            local = Path(directory) / 'region.pb'
            local.write_bytes(content)
            storage.send_file(str(local), TEMP_REGION_PATH)
            if storage.hash_flipper(TEMP_REGION_PATH).strip().lower() != expected_md5:
                raise RegionProvisioningError('Flipper region transfer checksum mismatch')

        had_previous = storage.exist_file(REGION_PATH)
        if had_previous:
            if storage.exist_file(BACKUP_REGION_PATH):
                storage.remove(BACKUP_REGION_PATH)
            _storage_command(
                storage, f'storage rename "{REGION_PATH}" "{BACKUP_REGION_PATH}"',
                REGION_PATH,
            )
        try:
            _storage_command(
                storage, f'storage rename "{TEMP_REGION_PATH}" "{REGION_PATH}"',
                REGION_PATH,
            )
        except Exception:
            if had_previous:
                _storage_command(
                    storage, f'storage rename "{BACKUP_REGION_PATH}" "{REGION_PATH}"',
                    REGION_PATH,
                )
            raise
        # Verify destination before discarding the rollback copy.
        if storage.hash_flipper(REGION_PATH).strip().lower() != expected_md5:
            if storage.exist_file(REGION_PATH):
                storage.remove(REGION_PATH)
            if had_previous:
                _storage_command(
                    storage, f'storage rename "{BACKUP_REGION_PATH}" "{REGION_PATH}"',
                    REGION_PATH,
                )
            raise RegionProvisioningError('Final region checksum differs from official data')
        if storage.exist_file(BACKUP_REGION_PATH):
            storage.remove(BACKUP_REGION_PATH)
        return True
    finally:
        # Only temporary material may be cleaned here; never remove an old
        # valid region when serial upload failed midway.
        try:
            if storage.exist_file(TEMP_REGION_PATH):
                storage.remove(TEMP_REGION_PATH)
        except Exception:
            pass  # USB link may already be gone; recovery is safe on next run.


def provision(storage, logger=None, force=False):
    """Provision regional rules using server-side geolocation; return (country, changed)."""
    logger = logger or logging.getLogger('WeaveRegion')
    if not force and storage.exist_file(REGION_PATH) and storage.size(REGION_PATH) > 0:
        logger.info('Sub-GHz region file already present in Flipper internal storage')
        return None, False
    country, content = fetch_official_region()
    logger.info('Flipper official region server selected %s (host network location)', country)
    logger.info('If location is incorrect, disconnect VPN/proxy and retry')
    changed = provision_storage(storage, country, content, force=force)
    if changed:
        logger.info('Installed official Sub-GHz region for %s in %s', country, REGION_PATH)
    return country, changed
