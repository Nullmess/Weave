
// Importer event_loop puis gui puis ses sous-modules

import * as eventLoop from "@flipperdevices/fz-sdk/event_loop";
import * as gui from "@flipperdevices/fz-sdk/gui";
import * as dialog from "@flipperdevices/fz-sdk/gui/dialog";


const views = {
    dialog: dialog.makeWith({
        header: "Hello from <app_name>",
        text: "Check out index.ts and\nchange something :)",
        center: "Gonna do that!",
    }),
};


eventLoop.subscribe(views.dialog.input, (_sub, button, eventLoop) => {
    if (button === "center")
        eventLoop.stop();
}, eventLoop);


eventLoop.subscribe(gui.viewDispatcher.navigation, (_sub, _item, eventLoop) => {
    eventLoop.stop();
}, eventLoop);


gui.viewDispatcher.switchTo(views.dialog);
eventLoop.run();
