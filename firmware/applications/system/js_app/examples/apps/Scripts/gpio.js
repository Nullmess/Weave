let eventLoop = require("event_loop");
let gpio = require("gpio");


let led = gpio.get("pc3"); // Équivalent gpio.get(7)
let led2 = gpio.get("pa7"); // Équivalent gpio.get(2)
let pot = gpio.get("pc0"); // Équivalent gpio.get(16)
let button = gpio.get("pc1"); // Équivalent gpio.get(15)
led.init({ direction: "out", outMode: "push_pull" });
pot.init({ direction: "in", inMode: "analog" });
button.init({ direction: "in", pull: "up", inMode: "interrupt", edge: "falling" });


print("Commencing blinking (PC3)");
eventLoop.subscribe(eventLoop.timer("periodic", 1000), function (_, _item, led, state) {
    led.write(state);
    return [led, !state];
}, led, true);


print("Commencing PWM (PA7)");
eventLoop.subscribe(eventLoop.timer("periodic", 10), function (_, _item, led2, state) {
    led2.pwmWrite(10000, state);
    return [led2, (state + 1) % 101];
}, led2, 0);


print("Press the button (PC1)");
eventLoop.subscribe(button.interrupt(), function (_, _item, pot) {
    print("PC0 is at", pot.readAnalog(), "mV");
}, pot);

// Event loop requis pour maintenir programme actif
eventLoop.run();

// Pins : https://docs.flipper.net/gpio-and-modules#miFsS
// "PA7" équivaut à 2
// "PA6" équivaut à 3
// "PA4" équivaut à 4
// "PB3" équivaut à 5
// "PB2" équivaut à 6
// "PC3" équivaut à 7
// "PA14" équivaut à 10
// "PA13" équivaut à 12
// "PB6" équivaut à 13
// "PB7" équivaut à 14
// "PC1" équivaut à 15
// "PC0" équivaut à 16
// "PB14" équivaut à 17


// { direction: "out", outMode: "push_pull" }
// { direction: "out", outMode: "open_drain" }
// { direction: "out", outMode: "push_pull", altFn: true }
// { direction: "out", outMode: "open_drain", altFn: true }
// { direction: "in", inMode: "analog" }
// { direction: "in", inMode: "plain_digital" }
// { direction: "in", inMode: "interrupt", edge: "rising" }
// { direction: "in", inMode: "interrupt", edge: "falling" }
// { direction: "in", inMode: "interrupt", edge: "both" }
// { direction: "in", inMode: "event", edge: "rising" }
// { direction: "in", inMode: "event", edge: "falling" }
// { direction: "in", inMode: "event", edge: "both" }
// Champ pull optionnel : undefined, up ou down
