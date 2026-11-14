let eventLoop = require("event_loop");
let gui = require("gui");
let dialog = require("gui/dialog");
let textInput = require("gui/text_input");
let loading = require("gui/loading");
let storage = require("storage");

// Sans eval/exec : charger fichier au nom unique
storage.makeDirectory("/ext/.tmp");
storage.makeDirectory("/ext/.tmp/js");
storage.rmrf("/ext/.tmp/js/repl")
storage.makeDirectory("/ext/.tmp/js/repl")
let ctx = {
    tmpTemplate: "/ext/.tmp/js/repl/",
    tmpNumber: 0,
    persistentScope: {},
};

let views = {
    dialog: dialog.makeWith({
        header: "Interactive Console",
        text: "Press OK to Start",
        center: "Run Some JS"
    }),
    textInput: textInput.makeWith({
        header: "Type JavaScript Code:",
        minLength: 0,
        maxLength: 256,
        defaultText: "2+2",
        defaultTextClear: true,
    }),
    loading: loading.make(),
};

eventLoop.subscribe(views.dialog.input, function (_sub, button, gui, views) {
    if (button === "center") {
        gui.viewDispatcher.switchTo(views.textInput);
    }
}, gui, views);

eventLoop.subscribe(views.textInput.input, function (_sub, text, gui, views, ctx) {
    gui.viewDispatcher.switchTo(views.loading);

    let path = ctx.tmpTemplate + (ctx.tmpNumber++).toString();
    let file = storage.openFile(path, "w", "create_always");
    file.write(text);
    file.close();

    // Masquer GUI pour console visible et éviter deadlock en erreur
    gui.viewDispatcher.sendTo("back");
    let result = load(path, ctx.persistentScope); // Load exécute JS, renvoie dernière valeur stack
    storage.remove(path);

    // Conversion explicite en string requise
    if (result === null) { // mJS typeof null=null ; ECMAScript object, avertissement IDE attendu
        result = "null";
    } else if (typeof result === "string") {
        result = "'" + result + "'";
    } else if (typeof result === "number") {
        result = result.toString();
    } else if (typeof result === "bigint") { // BigInt non supporté par mJS
        result = "bigint";
    } else if (typeof result === "boolean") {
        result = result ? "true" : "false";
    } else if (typeof result === "symbol") { // Symbol non supporté par mJS
        result = "symbol";
    } else if (typeof result === "undefined") {
        result = "undefined";
    } else if (typeof result === "object") {
        result = "object"; // JSON.stringify non implémenté
    } else if (typeof result === "function") {
        result = "function";
    } else {
        result = "unknown type: " + typeof result;
    }

    gui.viewDispatcher.sendTo("front");
    views.dialog.set("header", "JS Returned:");
    views.dialog.set("text", result);
    gui.viewDispatcher.switchTo(views.dialog);
    views.textInput.set("defaultText", text);
}, gui, views, ctx);

eventLoop.subscribe(gui.viewDispatcher.navigation, function (_sub, _, eventLoop) {
    eventLoop.stop();
}, eventLoop);

gui.viewDispatcher.switchTo(views.dialog);


print("If you're stuck here, something went wrong, re-run the script")
eventLoop.run();
print("\n\nFinished correctly :)")
