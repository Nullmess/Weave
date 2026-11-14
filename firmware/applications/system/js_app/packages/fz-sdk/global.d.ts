// SDK major : changement incompatible ; minor : fonctionnalité compatible
// Compatibilité TypeScript selon semver-ts.org
// Extra features amont deviennent baseline au prochain major
// Vérifier compatibilité et fonctionnalités avant utilisation
// Check automatique configurable via fz-sdk.config.json5

































































// compatible si versions admises ; firmwareTooOld si major/minor insuffisant
// firmwareTooNew si major firmware supérieur















declare function sdkCompatibilityStatus(expectedMajor: number, expectedMinor: number):
    "compatible" | "firmwareTooOld" | "firmwareTooNew";

// true si versions compatibles










declare function isSdkCompatible(expectedMajor: number, expectedMinor: number): boolean;

// Version incompatible : demander poursuite utilisateur









declare function checkSdkCompatibility(expectedMajor: number, expectedMinor: number): void | never;

// Baseline feature renvoie false, même si présente








declare function doesSdkSupport(features: string[]): boolean;

// Baseline feature traitée comme non implémentée









declare function checkSdkFeatures(features: string[]): void | never;

// Durée en millisecondes




declare function delay(ms: number): void;

// Arguments convertis en strings, concaténés sans espaces





declare function print(...args: any[]): void;

// Base 2..16, défaut 10





declare function parseInt(text: string, base?: number): number;





declare const __dirname: string;





declare const __filename: string;

// Dernière valeur évaluée renvoyée ; fichier cached pour session











declare function load(path: string, scope?: object): any;

// ASCII 0..255 ; null si n invalide/non numérique







declare function chr(n: number): string | null;






declare function require(module: string): any;






declare function die(message: string): never;

// RawPointer opaque : transmettre uniquement entre fonctions natives









declare type RawPointer = symbol & { "__tag__": "raw_ptr" };
// Type nominal : propriété __tag__ inexistante au runtime





declare class ArrayBuffer {
    // Pointeur opaque, inutilisable directement depuis JS





    getPtr: RawPointer;
    // Longueur en octets



    byteLength: number;
    // Slice [start,end[ ; end exclu







    slice(start: number, end?: number): ArrayBuffer;
}

declare function ArrayBuffer(): ArrayBuffer;

declare type ElementType = "u8" | "i8" | "u16" | "i16" | "u32" | "i32";

declare class TypedArray<E extends ElementType> {
    // Longueur en octets



    byteLength: number;
    // Longueur en éléments typés



    length: number;




    buffer: ArrayBuffer;
}

declare class Uint8Array extends TypedArray<"u8"> { }
declare class Int8Array extends TypedArray<"i8"> { }
declare class Uint16Array extends TypedArray<"u16"> { }
declare class Int16Array extends TypedArray<"i16"> { }
declare class Uint32Array extends TypedArray<"u32"> { }
declare class Int32Array extends TypedArray<"i32"> { }

declare function Uint8Array(data: ArrayBuffer | number | number[]): Uint8Array;
declare function Int8Array(data: ArrayBuffer | number | number[]): Int8Array;
declare function Uint16Array(data: ArrayBuffer | number | number[]): Uint16Array;
declare function Int16Array(data: ArrayBuffer | number | number[]): Int16Array;
declare function Uint32Array(data: ArrayBuffer | number | number[]): Uint32Array;
declare function Int32Array(data: ArrayBuffer | number | number[]): Int32Array;

declare const console: {
    // Logs niveau I ; arguments concaténés sans espaces





    log(...args: any[]): void;
    // Logs niveau D ; arguments concaténés sans espaces





    debug(...args: any[]): void;
    // Logs niveau W ; arguments concaténés sans espaces





    warn(...args: any[]): void;
    // Logs niveau E ; arguments concaténés sans espaces





    error(...args: any[]): void;
};

declare class Array<T> {
    // Renvoie nouvel array des éléments retirés










    splice(start: number, deleteCount: number): T[];
    // Renvoie nouvelle longueur array





    push(value: T): number;




    length: number;
}

declare class String {




    length: number;





    charCodeAt(index: number): number;




    at(index: number): number;
    // Index première occurrence, -1 si absente





    indexOf(substr: string, fromIndex?: number): number;
    // Substring entre deux indexes





    slice(start: number, end?: number): string;




    toUpperCase(): string;




    toLowerCase(): string;
}

declare class Boolean { }

declare class Function { }

declare class Number {
    // Base 2..16, défaut 10




    toString(base?: number): string;
}

declare class Object { }

declare class RegExp { }

declare interface IArguments { }

declare type Partial<O extends object> = { [K in keyof O]?: O[K] };
