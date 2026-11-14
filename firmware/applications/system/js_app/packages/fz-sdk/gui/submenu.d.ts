// Ordre imports event_loop→gui→submenu ; API changée SDK0.4

























import type { View, ViewFactory } from ".";
import type { Contract } from "../event_loop";

type Props = {
    header: string,
};
type Child = string;
declare class Submenu extends View<Props, Child> {
    chosen: Contract<number>;
}
declare class SubmenuFactory extends ViewFactory<Props, Child, Submenu> { }
declare const factory: SubmenuFactory;
export = factory;
