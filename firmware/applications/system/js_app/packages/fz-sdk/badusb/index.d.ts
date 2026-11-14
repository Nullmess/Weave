// Key codes reconnus, SDK 0.1



export type ModifierKey = "CTRL" | "SHIFT" | "ALT" | "GUI";


export type MainKey =
    "DOWN" | "LEFT" | "RIGHT" | "UP" |

    "ENTER" | "PAUSE" | "CAPSLOCK" | "DELETE" | "BACKSPACE" | "END" | "ESC" |
    "HOME" | "INSERT" | "NUMLOCK" | "PAGEUP" | "PAGEDOWN" | "PRINTSCREEN" |
    "SCROLLLOCK" | "SPACE" | "TAB" | "MENU" |

    "F1" | "F2" | "F3" | "F4" | "F5" | "F6" | "F7" | "F8" | "F9" | "F10" |
    "F11" | "F12" | "F13" | "F14" | "F15" | "F16" | "F17" | "F18" | "F19" |
    "F20" | "F21" | "F22" | "F23" | "F24" |

    "NUM0" | "NUM1" | "NUM2" | "NUM3" | "NUM4" | "NUM5" | "NUM6" | "NUM7" |
    "NUM8" | "NUM9" |

    "\n" | " " | "!" | "\"" | "#" | "$" | "%" | "&" | "'" | "(" | ")" | "*" |
    "+" | "," | "-" | "." | "/" | ":" | ";" | "<" | ">" | "=" | "?" | "@" | "[" |
    "]" | "\\" | "^" | "_" | "`" | "{" | "}" | "|" | "~" |

    "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" |

    "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" | "J" | "K" | "L" |
    "M" | "N" | "O" | "P" | "Q" | "R" | "S" | "T" | "U" | "V" | "W" | "X" |
    "Y" | "Z" |

    "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" | "j" | "k" | "l" |
    "m" | "n" | "o" | "p" | "q" | "r" | "s" | "t" | "u" | "v" | "w" | "x" |
    "y" | "z";


export type KeyCode = MainKey | ModifierKey | number;

// settings omis : configuration USB par défaut




export declare function setup(settings?: { vid: number, pid: number, mfrName?: string, prodName?: string, layoutPath?: string }): void;





export declare function isConnected(): boolean;

// Une MainKey maximum, autres touches ModifierKey






export declare function press(...keys: KeyCode[]): void;

// Une MainKey maximum, autres touches ModifierKey






export declare function hold(...keys: KeyCode[]): void;

// Une MainKey maximum, autres touches ModifierKey






export declare function release(...keys: KeyCode[]): void;

// delay en millisecondes entre touches





export declare function print(string: string, delay?: number): void;

// Ajoute Enter après string ; delay en ms






export declare function println(string: string, delay?: number): void;

// Alt+Numpad Windows uniquement ; delay en ms





export declare function altPrint(string: string, delay?: number): void;

// Alt+Numpad Windows uniquement, Enter final ; delay en ms






export declare function altPrintln(string: string, delay?: number): void;

// Libère USB pour changement profil



export declare function quit(): void;
