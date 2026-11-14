let eventLoop = require("event_loop");

// String après 1337 ms
eventLoop.subscribe(eventLoop.timer("oneshot", 1337), function (_subscription, _item) {
    print("Hi after 1337 ms");
});

// Compteur jusqu’à 5, intervalle 100 ms
eventLoop.subscribe(eventLoop.timer("periodic", 100), function (subscription, _item, counter) {
    print("Counter two:", counter);
    if (counter === 5)
        subscription.cancel();
    return [counter + 1];
}, 0);

// Compteur jusqu’à 15, intervalle 100 ms

eventLoop.subscribe(eventLoop.timer("periodic", 100), function (subscription, _item, event_loop, counter) {
    print("Counter one:", counter);
    if (counter === 15)
        event_loop.stop();
    return [event_loop, counter + 1];
}, eventLoop, 0);

eventLoop.run();
