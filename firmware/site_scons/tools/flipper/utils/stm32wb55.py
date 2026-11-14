import logging
from enum import Enum

from flipper.utils.openocd import OpenOCD
from flipper.utils.register import Register32, RegisterBitDefinition


class STM32WB55:
    # Adresse mémoire OTP en flash
    OTP_BASE = 0x1FFF7000

    # Adresse Option Bytes en flash
    OPTION_BYTE_BASE = 0x1FFF8000


    FLASH_BASE = 0x58004000


    FLASH_KEYR = FLASH_BASE + 0x08


    FLASH_OPTKEYR = FLASH_BASE + 0x0C


    FLASH_UNLOCK_KEY1 = 0x45670123
    FLASH_UNLOCK_KEY2 = 0xCDEF89AB


    FLASH_UNLOCK_OPTKEY1 = 0x08192A3B
    FLASH_UNLOCK_OPTKEY2 = 0x4C5D6E7F


    FLASH_CR = Register32(
        FLASH_BASE + 0x14,
        [
            RegisterBitDefinition("PG", 0, 1),
            RegisterBitDefinition("PER", 1, 1),
            RegisterBitDefinition("MER", 2, 1),
            RegisterBitDefinition("PNB", 3, 8),
            RegisterBitDefinition("_", 11, 5),
            RegisterBitDefinition("STRT", 16, 1),
            RegisterBitDefinition("OPT_STRT", 17, 1),
            RegisterBitDefinition("FSTPG", 18, 1),
            RegisterBitDefinition("_", 19, 5),
            RegisterBitDefinition("EOPIE", 24, 1),
            RegisterBitDefinition("ERRIE", 25, 1),
            RegisterBitDefinition("RD_ERRIE", 26, 1),
            RegisterBitDefinition("OBL_LAUNCH", 27, 1),
            RegisterBitDefinition("_", 28, 2),
            RegisterBitDefinition("OPT_LOCK", 30, 1),
            RegisterBitDefinition("LOCK", 31, 1),
        ],
    )


    FLASH_SR = Register32(
        FLASH_BASE + 0x10,
        [
            RegisterBitDefinition("EOP", 0, 1),
            RegisterBitDefinition("OP_ERR", 1, 1),
            RegisterBitDefinition("_", 2, 1),
            RegisterBitDefinition("PROG_ERR", 3, 1),
            RegisterBitDefinition("WRP_ERR", 4, 1),
            RegisterBitDefinition("PGA_ERR", 5, 1),
            RegisterBitDefinition("SIZE_ERR", 6, 1),
            RegisterBitDefinition("PGS_ERR", 7, 1),
            RegisterBitDefinition("MISS_ERR", 8, 1),
            RegisterBitDefinition("FAST_ERR", 9, 1),
            RegisterBitDefinition("_", 10, 3),
            RegisterBitDefinition("OPTNV", 13, 1),
            RegisterBitDefinition("RD_ERR", 14, 1),
            RegisterBitDefinition("OPTV_ERR", 15, 1),
            RegisterBitDefinition("BSY", 16, 1),
            RegisterBitDefinition("_", 17, 1),
            RegisterBitDefinition("CFGBSY", 18, 1),
            RegisterBitDefinition("PESD", 19, 1),
            RegisterBitDefinition("_", 20, 12),
        ],
    )


    FLASH_OPTR = FLASH_BASE + 0x20
    FLASH_PCROP1ASR = FLASH_BASE + 0x24
    FLASH_PCROP1AER = FLASH_BASE + 0x28
    FLASH_WRP1AR = FLASH_BASE + 0x2C
    FLASH_WRP1BR = FLASH_BASE + 0x30
    FLASH_PCROP1BSR = FLASH_BASE + 0x34
    FLASH_PCROP1BER = FLASH_BASE + 0x38
    FLASH_IPCCBR = FLASH_BASE + 0x3C

    # Mapping index dword Option Bytes vers adresse registre
    OPTION_BYTE_MAP_TO_REGS = {
        0: FLASH_OPTR,
        1: FLASH_PCROP1ASR,
        2: FLASH_PCROP1AER,
        3: FLASH_WRP1AR,
        4: FLASH_WRP1BR,
        5: FLASH_PCROP1BSR,
        6: FLASH_PCROP1BER,
        7: None,  # Options invalides
        8: None,  # Options invalides
        9: None,  # Options invalides
        10: None,  # Options invalides
        11: None,  # Options invalides
        12: None,  # Options invalides
        13: FLASH_IPCCBR,
        14: None,
        15: None,
    }

    def __init__(self, openocd: OpenOCD):
        self.openocd = openocd
        self.logger = logging.getLogger("STM32WB55")

        self.FLASH_CR.set_openocd(self.openocd)
        self.FLASH_SR.set_openocd(self.openocd)

    class RunMode(Enum):
        Init = "init"
        Run = "run"
        Halt = "halt"

    def reset(self, mode: RunMode):
        self.logger.debug("Resetting device")
        self.openocd.send_tcl(f"reset {mode.value}")

    def clear_flash_errors(self):
        # Errata 2.2.9 : OPTVERR toujours actif après reset

        self.logger.debug("Resetting flash errors")
        self.FLASH_SR.load()
        self.FLASH_SR.OP_ERR = 1
        self.FLASH_SR.PROG_ERR = 1
        self.FLASH_SR.WRP_ERR = 1
        self.FLASH_SR.PGA_ERR = 1
        self.FLASH_SR.SIZE_ERR = 1
        self.FLASH_SR.PGS_ERR = 1
        self.FLASH_SR.MISS_ERR = 1
        self.FLASH_SR.FAST_ERR = 1
        self.FLASH_SR.RD_ERR = 1
        self.FLASH_SR.OPTV_ERR = 1
        self.FLASH_SR.store()

    def flash_unlock(self):

        self.FLASH_CR.load()
        if self.FLASH_CR.LOCK == 0:
            self.logger.debug("Flash is already unlocked")
            return


        self.logger.debug("Unlocking Flash")
        self.openocd.write_32(self.FLASH_KEYR, self.FLASH_UNLOCK_KEY1)
        self.openocd.write_32(self.FLASH_KEYR, self.FLASH_UNLOCK_KEY2)


        self.FLASH_CR.load()
        if self.FLASH_CR.LOCK == 0:
            self.logger.debug("Flash unlocked")
        else:
            self.logger.error("Flash unlock failed")
            raise Exception("Flash unlock failed")

    def option_bytes_unlock(self):

        self.FLASH_CR.load()
        if self.FLASH_CR.OPT_LOCK == 0:
            self.logger.debug("Options is already unlocked")
            return


        self.logger.debug("Unlocking Options")
        self.openocd.write_32(self.FLASH_OPTKEYR, self.FLASH_UNLOCK_OPTKEY1)
        self.openocd.write_32(self.FLASH_OPTKEYR, self.FLASH_UNLOCK_OPTKEY2)


        self.FLASH_CR.load()
        if self.FLASH_CR.OPT_LOCK == 0:
            self.logger.debug("Options unlocked")
        else:
            self.logger.error("Options unlock failed")
            raise Exception("Options unlock failed")

    def option_bytes_lock(self):

        self.FLASH_CR.load()
        if self.FLASH_CR.OPT_LOCK == 1:
            self.logger.debug("Options is already locked")
            return


        self.logger.debug("Locking Options")
        self.FLASH_CR.OPT_LOCK = 1
        self.FLASH_CR.store()


        self.FLASH_CR.load()
        if self.FLASH_CR.OPT_LOCK == 1:
            self.logger.debug("Options locked")
        else:
            self.logger.error("Options lock failed")
            raise Exception("Options lock failed")

    def flash_lock(self):

        self.FLASH_CR.load()
        if self.FLASH_CR.LOCK == 1:
            self.logger.debug("Flash is already locked")
            return


        self.logger.debug("Locking Flash")
        self.FLASH_CR.LOCK = 1
        self.FLASH_CR.store()


        self.FLASH_CR.load()
        if self.FLASH_CR.LOCK == 1:
            self.logger.debug("Flash locked")
        else:
            self.logger.error("Flash lock failed")
            raise Exception("Flash lock failed")

    def option_bytes_apply(self):
        self.logger.debug("Applying Option Bytes")

        self.FLASH_CR.load()
        self.FLASH_CR.OPT_STRT = 1
        self.FLASH_CR.store()


        self.flash_wait_for_operation()

    def option_bytes_load(self):
        self.logger.debug("Loading Option Bytes")
        self.FLASH_CR.load()
        self.FLASH_CR.OBL_LAUNCH = 1
        self.FLASH_CR.store()

    def option_bytes_id_to_address(self, id: int) -> int:
        # Vérifier mapping dword OB vers registre
        device_reg_addr = self.OPTION_BYTE_MAP_TO_REGS.get(id, None)
        if device_reg_addr is None:
            raise Exception(f"Option Byte {id} is not mapped to a register")

        return device_reg_addr

    def flash_wait_for_operation(self):

        # TODO FL-3537: gérer timeout
        while True:
            self.FLASH_SR.load()
            if self.FLASH_SR.BSY == 0:
                break

    def flash_dump_status_register(self):
        self.FLASH_SR.load()
        self.logger.info(f"FLASH_SR: {self.FLASH_SR.get():08x}")
        if self.FLASH_SR.EOP:
            self.logger.info("    End of operation")
        if self.FLASH_SR.OP_ERR:
            self.logger.error("    Operation error")
        if self.FLASH_SR.PROG_ERR:
            self.logger.error("    Programming error")
        if self.FLASH_SR.WRP_ERR:
            self.logger.error("    Write protection error")
        if self.FLASH_SR.PGA_ERR:
            self.logger.error("    Programming alignment error")
        if self.FLASH_SR.SIZE_ERR:
            self.logger.error("    Size error")
        if self.FLASH_SR.PGS_ERR:
            self.logger.error("    Programming sequence error")
        if self.FLASH_SR.MISS_ERR:
            self.logger.error("    Fast programming data miss error")
        if self.FLASH_SR.FAST_ERR:
            self.logger.error("    Fast programming error")
        if self.FLASH_SR.OPTNV:
            self.logger.info("    User option OPTVAL indication")
        if self.FLASH_SR.RD_ERR:
            self.logger.info("    PCROP read error")
        if self.FLASH_SR.OPTV_ERR:
            self.logger.info("    Option and Engineering bits loading validity error")
        if self.FLASH_SR.BSY:
            self.logger.info("    Busy")
        if self.FLASH_SR.CFGBSY:
            self.logger.info("    Programming or erase configuration busy")
        if self.FLASH_SR.PESD:
            self.logger.info("    Programming / erase operation suspended.")

    def write_flash_64(self, address: int, word_1: int, word_2: int):
        self.logger.debug(f"Writing flash at address {address:08x}")

        if address % 8 != 0:
            self.logger.error("Address must be aligned to 8 bytes")
            raise Exception("Address must be aligned to 8 bytes")

        if word_1 == self.openocd.read_32(address) and word_2 == self.openocd.read_32(
            address + 4
        ):
            self.logger.debug("Data is already programmed")
            return

        self.flash_unlock()

        # Attente fin opération Flash via BSY
        self.FLASH_SR.load()
        if self.FLASH_SR.BSY:
            self.logger.error("Flash is busy")
            self.flash_dump_status_register()
            raise Exception("Flash is busy")


        self.FLASH_CR.load()
        self.FLASH_CR.EOPIE = 1
        self.FLASH_CR.ERRIE = 1
        self.FLASH_CR.store()

        # Vérifier autorisation programmation/effacement flash
        if self.FLASH_SR.PESD:
            self.logger.error("Flash operations are not allowed")
            self.flash_dump_status_register()
            raise Exception("Flash operations are not allowed")

        # Effacer flags erreurs de programmation précédentes
        self.clear_flash_errors()

        # Activation PG dans FLASH_CR
        self.FLASH_CR.load()
        self.FLASH_CR.PG = 1
        self.FLASH_CR.store()

        # Programmation Flash uniquement par double word 64 bits
        # Écriture premier word
        self.openocd.send_tcl(f"mww 0x{address:08x} 0x{word_1:08x}")
        # Écriture second word
        self.openocd.send_tcl(f"mww 0x{(address + 4):08x} 0x{word_2:08x}")

        # Attente désactivation BSY
        self.flash_wait_for_operation()

        # Vérification EOP dans FLASH_SR
        self.FLASH_SR.load()
        if not self.FLASH_SR.EOP:
            self.logger.error("Flash operation failed")
            self.flash_dump_status_register()
            raise Exception("Flash operation failed")

        # Effacement EOP
        self.FLASH_SR.load()
        self.FLASH_SR.EOP = 1
        self.FLASH_SR.store()

        # Effacement PG dans FLASH_CR
        self.FLASH_CR.load()
        self.FLASH_CR.PG = 0
        self.FLASH_CR.store()

        self.flash_lock()

    def option_bytes_recover(self):
        self.openocd.send_tcl("mww 0x58004010 0x8000")  # Reset OPTVERR





        self.flash_unlock()
        self.option_bytes_unlock()
        self.openocd.send_tcl("mmw 0x58004020 0x3ffff1aa 0xffffffff")
        self.openocd.send_tcl("mww 0x5800402c 0xff")
        self.openocd.send_tcl("mww 0x58004030 0xff")
        self.openocd.send_tcl("mmw 0x58004014 0x00020000 0")  # OPTSTRT
        self.openocd.send_tcl("mmw 0x58004014 0x08000000 0")  # OBL_LAUNCH
