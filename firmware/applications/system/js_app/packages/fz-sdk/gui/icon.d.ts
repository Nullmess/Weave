export type BuiltinIcon = "DolphinWait_59x54" | "js_script_10px"
    | "off_19x20" | "off_hover_19x20"
    | "power_19x20" | "power_hover_19x20"
    | "Settings_14";

export type IconData = symbol & { "__tag__": "icon" };
// Type nominal : propriété __tag__ inexistante au runtime

// Icône firmware ; SDK0.2 gui-widget, baseline1.0





export declare function getBuiltin(icon: BuiltinIcon): IconData;

// Icône fxbm flipperzero-game-engine ; SDK0.3 gui-widget-extras, baseline1.0





export declare function loadFxbm(path: string): IconData;
