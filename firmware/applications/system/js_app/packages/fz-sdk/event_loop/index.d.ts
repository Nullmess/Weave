// Event loop à exécuter explicitement via run, arrêt via stop
// mJS sans closures : passer état externe comme arguments subscribe
// Callback reçoit subscription puis item, undefined pour timer
// Array retour même longueur : remplace arguments externes au prochain appel




























































































/**
 * @ignore
 */
type Lit = undefined | null | {};





export interface Subscription {
    // Cancel empêche tout event ultérieur de cet abonnement




    cancel(): void;
}





export type Contract<Item = undefined> = symbol & { "__tag__": "contract" };
// Type nominal : propriété __tag__ inexistante au runtime

// Callback reçoit subscription/item ; array retour réassigne état externe







export type Callback<Item, Args extends Lit[]> = (subscription: Subscription, item: Item, ...args: Args) => Args | undefined | void;








export function subscribe<Item, Args extends Lit[]>(contract: Contract<Item>, callback: Callback<Item, Args>, ...args: Args): Subscription;
// Run bloque jusqu’à stop, potentiellement indéfiniment



export function run(): void | never;




export function stop(): void;

// Mode oneshot/periodic ; intervalle en ms





export function timer(mode: "oneshot" | "periodic", interval: number): Contract;





export declare class Queue<T> {




    input: Contract<T>;





    send(message: T): void;
}

// Longueur définit capacité queue




export function queue<T>(length: number): Queue<T>;
