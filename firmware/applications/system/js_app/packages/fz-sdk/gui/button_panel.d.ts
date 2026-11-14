// Ordre imports event_loop→gui→sous-module ; grille X/Y ; SDK0.4

























import type { View, ViewFactory, Font, InputType } from ".";
import type { Contract } from "../event_loop";
import { IconData } from "./icon";

type Props = {
    matrixSizeX: number,
    matrixSizeY: number,
};

type Position = { x: number, y: number };

type ButtonChild = { type: "button", matrixX: number, matrixY: number, icon: IconData, iconSelected: IconData } & Position;
type LabelChild = { type: "label", font: Font, text: string } & Position;
type IconChild = { type: "icon", icon: IconData };

type Child = ButtonChild | LabelChild | IconChild;

declare class ButtonPanel extends View<Props, Child> {
    input: Contract<{ index: number, type: InputType }>;
}
declare class ButtonPanelFactory extends ViewFactory<Props, Child, ButtonPanel> { }
declare const factory: ButtonPanelFactory;
export = factory;
