// Ordre imports event_loop→gui→sous-module ; longueur octets édités


























import type { View, ViewFactory } from ".";
import type { Contract } from "../event_loop";

type Props = {
    header: string,
    length: number,
    defaultData: Uint8Array | ArrayBuffer,
}
type Child = never;
declare class ByteInput extends View<Props, Child> {
    input: Contract<string>;
}
declare class ByteInputFactory extends ViewFactory<Props, Child, ByteInput> { }
declare const factory: ByteInputFactory;
export = factory;
