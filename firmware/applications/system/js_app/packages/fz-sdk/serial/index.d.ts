





export interface Framing {
    // 6 bits nécessite parité ; 9 bits nécessite absence parité




    dataBits: "6" | "7" | "8" | "9";
    parity: "none" | "even" | "odd";
    // LPUART stop bits entiers 1/2 uniquement



    stopBits: "0.5" | "1" | "1.5" | "2";
}

// Init désactive service Expansion ; framing SDK0.3/baseline1.0











export declare function setup(port: "lpuart" | "usart", baudRate: number, framing?: Framing): void;

// Strings ASCII ; nombre un octet ; arrays/buffers séquences octets









export declare function write<E extends ElementType>(value: string | number | number[] | ArrayBuffer | TypedArray<E>): void;

// ASCII ou undefined si vide ; timeout ms, sinon attente infinie









export declare function read(length: number, timeout?: number): string | undefined;

// ASCII jusqu’à CR/LF exclus ; undefined si vide
// Timeout ms par caractère, pas string ; défaut infini












export declare function readln(timeout?: number): string;

// readAny évite starvation ; timeout ms, défaut infini ; undefined si vide











export declare function readAny(timeout?: number): string | undefined;

// ArrayBuffer ou undefined si vide ; timeout ms, défaut infini









export declare function readBytes(length: number, timeout?: number): ArrayBuffer;

// Match string, bytes, ou liste de patterns
// Index pattern si multiple, 0 si unique, undefined sinon
// Timeout ms par caractère, pas string ; défaut infini



















export declare function expect(patterns: string | number[] | string[] | number[][], timeout?: number): number | undefined;

// End autorise reconfiguration dans même script



export declare function end(): void;
