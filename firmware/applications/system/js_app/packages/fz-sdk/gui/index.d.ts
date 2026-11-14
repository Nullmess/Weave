// Importer event_loop avant gui et ses sous-modules
// makeWith initialise properties ; set les modifie
// ViewDispatcher disponible JS ; SceneManager/ViewPort non exposés



















































































































import type { Contract } from "../event_loop";

export type Font = "primary" | "secondary" | "keyboard" | "big_numbers";
export type InputType = "press" | "release" | "short" | "long" | "repeat";

type Properties = { [K: string]: any };

export declare class View<Props extends Properties, Child> {






    set<P extends keyof Props>(property: P, value: Props[P]): void;
    // Child View ; SDK0.2 gui-widget, baseline1.0





    addChild<C extends Child>(child: C): void;
    // Retire children ; SDK0.2 gui-widget, baseline1.0




    resetChildren(): void;
    // Remplace children ; SDK0.2 gui-widget, baseline1.0





    setChildren(children: Child[]): void;
}

export declare class ViewFactory<Props extends Properties, Child, V extends View<Props, Child>> {
    // Defaults ensuite modifiables via set



    make(): V;
    // Initial properties personnalisées, children optionnels ; set ensuite possible







    makeWith(initial: Partial<Props>, children?: Child[]): V;
}




declare class ViewDispatcher {




    custom: Contract<number>;
    // Event navigation sur Back



    navigation: Contract;




    currentView: View<any, any>;





    sendCustom(event: number): void;





    switchTo(assoc: View<any, any>): void;
    // front/back place ViewDispatcher devant/derrière tous viewports





    sendTo(direction: "front" | "back"): void;
}




export const viewDispatcher: ViewDispatcher;
