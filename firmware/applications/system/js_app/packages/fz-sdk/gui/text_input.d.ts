// Ordre imports event_loop→gui→text_input ; clear au prochain caractère




























import type { View, ViewFactory } from ".";
import type { Contract } from "../event_loop";

type Props = {
    header: string,
    minLength: number,
    maxLength: number,
    defaultText: string,
    defaultTextClear: boolean,
}
type Child = never;
declare class TextInput extends View<Props, Child> {
    input: Contract<string>;
}
declare class TextInputFactory extends ViewFactory<Props, Child, TextInput> { }
declare const factory: TextInputFactory;
export = factory;
