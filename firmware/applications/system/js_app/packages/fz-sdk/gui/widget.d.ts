// Ordre imports event_loop→gui→widget ; children, SDK0.2/baseline1.0




























import type { View, ViewFactory } from ".";
import type { IconData } from "./icon";
import type { Contract } from "../event_loop";

type Position = { x: number, y: number };
type Size = { w: number, h: number };
type Alignment = { align: `${"t" | "c" | "b"}${"l" | "m" | "r"}` };
type Font = { font: "primary" | "secondary" | "keyboard" | "big_numbers" };
type Text = { text: string };

type StringMultilineElement = { element: "string_multiline" } & Position & Alignment & Font & Text;
type StringElement = { element: "string" } & Position & Alignment & Font & Text;
type TextBoxElement = { element: "text_box", stripToDots: boolean } & Position & Size & Alignment & Text;
type TextScrollElement = { element: "text_scroll" } & Position & Size & Text;
type ButtonElement = { element: "button", button: "left" | "center" | "right" } & Text;
type IconElement = { element: "icon", iconData: IconData } & Position;
// SDK0.3 gui-widget-extras ; baseline1.0



type RectElement = { element: "rect", radius: number, fill: boolean } & Position & Size;
// SDK0.3 gui-widget-extras ; baseline1.0



type CircleElement = { element: "circle", radius: number, fill: boolean } & Position;
// SDK0.3 gui-widget-extras ; baseline1.0



type LineElement = { element: "line", x1: number, y1: number, x2: number, y2: number };

type Element = StringMultilineElement
    | StringElement
    | TextBoxElement
    | TextScrollElement
    | ButtonElement
    | IconElement
    | RectElement
    | CircleElement
    | LineElement;

type Props = {};
type Child = Element;
declare class ButtonEvent {
    key: "left" | "center" | "right";
    type: "press" | "release" | "short" | "long" | "repeat";
}
declare class Widget extends View<Props, Child> {
    // Event bouton seulement si élément bouton correspondant



    button: Contract<ButtonEvent>;
}
declare class WidgetFactory extends ViewFactory<Props, Child, Widget> { }
declare const factory: WidgetFactory;
export = factory;
