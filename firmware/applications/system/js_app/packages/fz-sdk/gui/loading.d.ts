// Ordre imports event_loop→gui→loading ; Back navigation supprimée

























import type { View, ViewFactory } from ".";

type Props = {};
type Child = never;
declare class Loading extends View<Props, Child> { }
declare class LoadingFactory extends ViewFactory<Props, Child, Loading> { }
declare const factory: LoadingFactory;
export = factory;
