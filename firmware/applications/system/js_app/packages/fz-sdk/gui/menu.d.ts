// Ordre imports event_loop→gui→menu ; API changée SDK0.4

























import type { View, ViewFactory } from ".";
import type { Contract } from "../event_loop";
import type { IconData } from "./icon";

type Props = {};
type Child = { icon: IconData, label: string };
declare class Submenu extends View<Props, Child> {
    chosen: Contract<number>;
}
declare class SubmenuFactory extends ViewFactory<Props, Child, Submenu> { }
declare const factory: SubmenuFactory;
export = factory;
