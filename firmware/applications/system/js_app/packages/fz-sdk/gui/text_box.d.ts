// Ordre imports event_loop→gui→text_box ; font text/hex, focus start/end


























import type { View, ViewFactory } from ".";
import type { Contract } from "../event_loop";

type Props = {
    text: string,
    font: "text" | "hex",
    focus: "start" | "end",
}
type Child = never;
declare class TextBox extends View<Props, Child> {
    chosen: Contract<number>;
}
declare class TextBoxFactory extends ViewFactory<Props, Child, TextBox> { }
declare const factory: TextBoxFactory;
export = factory;
