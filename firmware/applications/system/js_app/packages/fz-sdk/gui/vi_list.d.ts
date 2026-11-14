// Ordre imports event_loop→gui→vi_list ; SDK0.4
























import type { View, ViewFactory } from ".";
import type { Contract } from "../event_loop";

type Props = {};

type Child = { label: string, variants: string[] };

declare class ViList extends View<Props, Child> {
    valueUpdate: Contract<{ itemIndex: number, valueIndex: number }>;
}
declare class ViListFactory extends ViewFactory<Props, Child, ViList> { }
declare const factory: ViListFactory;
export = factory;
