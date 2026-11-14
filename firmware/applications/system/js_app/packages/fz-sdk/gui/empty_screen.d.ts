// Ordre imports event_loop→gui→sous-module ; aucune property
























import type { View, ViewFactory } from ".";

type Props = {};
type Child = never;
declare class EmptyScreen extends View<Props, Child> { }
declare class EmptyScreenFactory extends ViewFactory<Props, Child, EmptyScreen> { }
declare const factory: EmptyScreenFactory;
export = factory;
