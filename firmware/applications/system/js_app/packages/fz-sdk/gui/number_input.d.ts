// Ordre imports event_loop→gui→number_input ; SDK0.4



























import type { View, ViewFactory } from ".";
import type { Contract } from "../event_loop";

type Props = {
    header: string,
    minValue: number,
    maxValue: number,
    defaultValue: number,
}
type Child = never;
declare class NumberInput extends View<Props, Child> {
    input: Contract<number>;
}
declare class NumberInputFactory extends ViewFactory<Props, Child, NumberInput> { }
declare const factory: NumberInputFactory;
export = factory;
