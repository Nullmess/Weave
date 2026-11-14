// Ordre imports event_loop→gui→sous-module ; SDK0.4
























import type { View, ViewFactory, InputType } from ".";
import type { Contract } from "../event_loop";

type Props = {
    header: string;
};

type Child = { type: "common" | "control", label: string };

declare class ButtonMenu extends View<Props, Child> {
    input: Contract<{ index: number, type: InputType }>;
}
declare class ButtonMenuFactory extends ViewFactory<Props, Child, ButtonMenu> { }
declare const factory: ButtonMenuFactory;
export = factory;
