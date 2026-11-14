// UART 8E1 : huit bits, parité paire, un stop
// Contrairement au défaut 8N1 sans parité


let serial = require("serial");
serial.setup("usart", 230400, { dataBits: "8", parity: "even", stopBits: "1" });

while (1) {
    let rx_data = serial.readBytes(1, 1000);
    if (rx_data !== undefined) {
        serial.write(rx_data);
        let data_view = Uint8Array(rx_data);
        print("0x" + data_view[0].toString(16));
    }
}
