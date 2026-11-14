// Ordre imports event_loop→gui→popup ; timeout ms démarre à assignation



























import type { View, ViewFactory } from ".";
import type { Contract } from "../event_loop";

type Props = {
    header: string,
    text: string,
    timeout: number,
}
type Child = never;
declare class Popup extends View<Props, Child> {
    timeout: Contract;
}
declare class PopupFactory extends ViewFactory<Props, Child, Popup> { }
declare const factory: PopupFactory;
export = factory;
