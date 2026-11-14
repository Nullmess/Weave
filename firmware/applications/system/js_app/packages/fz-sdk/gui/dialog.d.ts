// Ordre imports event_loop→gui→sous-module ; trois boutons maximum




























import type { View, ViewFactory } from ".";
import type { Contract } from "../event_loop";

type Props = {
    header: string,
    text: string,
    left: string,
    center: string,
    right: string,
}
type Child = never;
declare class Dialog extends View<Props, Child> {
    input: Contract<"left" | "center" | "right">;
}
declare class DialogFactory extends ViewFactory<Props, Child, Dialog> { }
declare const factory: DialogFactory;
export = factory;
