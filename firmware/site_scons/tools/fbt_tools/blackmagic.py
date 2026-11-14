from SCons.Errors import StopError


class BlackmagicResolver:
    BLACKMAGIC_HOSTNAME = "blackmagic.local"

    def __init__(self, env):
        self.env = env

    # Sous Windows :
    #    'location': '1-5:x.0', 'name': 'COM4',
    #    'location': '1-5:x.2', 'name': 'COM13',
    # Sous Linux :
    #    'location': '1-1.2:1.0', 'name': 'ttyACM0',
    #    'location': '1-1.2:1.2', 'name': 'ttyACM1',
    # Sous macOS :
    #    'location': '0-1.3', 'name': 'cu.usbmodemblackmagic1',
    #    'location': '0-1.3', 'name': 'cu.usbmodemblackmagic3',
    def _find_probe(self):
        import serial.tools.list_ports as list_ports

        ports = list(list_ports.grep("blackmagic"))
        if len(ports) == 0:
            # Port Blackmagic absent : traitement différé
            pass
        elif len(ports) > 2:
            raise StopError("More than one Blackmagic probe found")
        else:


            return sorted(ports, key=lambda p: f"{p.location}_{p.name}")[0]

    # Résolution DNS de la sonde Blackmagic
    def _resolve_hostname(self):
        import socket

        try:
            return socket.gethostbyname(self.BLACKMAGIC_HOSTNAME)
        except socket.gaierror:
            print("Failed to resolve Blackmagic hostname")
            return None

    def get_serial(self):
        if not (probe := self._find_probe()):
            return None

        if self.env.subst("$PLATFORM") == "win32":
            return f"\\\\.\\{probe.device}"
        return probe.device

    def get_networked(self):
        if not (probe := self._resolve_hostname()):
            return None

        return f"tcp:{probe}:2345"

    def __str__(self):

        if (blackmagic := self.env.subst("$BLACKMAGIC")) != "auto":
            return blackmagic


        if probe := self.get_serial() or self.get_networked():
            return probe

        raise StopError("Please specify BLACKMAGIC=...")


def generate(env):
    env.SetDefault(BLACKMAGIC_ADDR=BlackmagicResolver(env))


def exists(env):
    return True
