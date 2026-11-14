





// r lecture ; w écriture ; rw lecture/écriture






export type AccessMode = "r" | "w" | "rw";

// open_existing exige existence ; open_always crée si absent
// open_append curseur EOF ; crée si absent
// create_new exige absence ; create_always tronque ou crée






export type OpenMode = "open_existing" | "open_always" | "open_append" | "create_new" | "create_always";

// Timestamp UNIX standard



export type Timestamp = number;





export declare class FileInfo {
    // stat : chemin complet ; readDirectory : nom seul




    path: string;




    isDirectory: boolean;
    // Taille octets ; dossier : zéro



    size: number;
    // Dernier accès en timestamp UNIX



    accessTime: Timestamp;
}





export declare class FsInfo {
    // Taille totale en octets



    totalSpace: number;
    // Espace libre en octets



    freeSpace: number;
}







export declare class File {
    // Après close, autres opérations fichier indisponibles





    close(): boolean;




    isOpen(): boolean;
    // Lecture r/rw : ASCII ou binary ; quantité réelle potentiellement partielle









    read<T extends ArrayBuffer | string>(mode: T extends ArrayBuffer ? "binary" : "ascii", bytes: number): T;
    // Écriture w/rw : ASCII ou ArrayBuffer ; octets réellement écrits






    write(data: ArrayBuffer | string): number;






    seekRelative(bytes: number): boolean;






    seekAbsolute(bytes: number): boolean;
    // Position absolue en octets



    tell(): number;
    // Fichier w/rw : tronque après curseur courant





    truncate(): boolean;
    // Taille fichier en octets



    size(): number;




    eof(): boolean;
    // Copie entre positions courantes des deux fichiers







    copyTo(dest: File, bytes: number): boolean;
}

// File si succès ; undefined si échec








export declare function openFile(path: string, accessMode: AccessMode, openMode: OpenMode): File | undefined;






export declare function fileExists(path: string): boolean;



// Array FileInfo ou undefined si erreur






export declare function readDirectory(path: string): FileInfo[] | undefined;





export declare function directoryExists(path: string): boolean;






export declare function makeDirectory(path: string): boolean;








export declare function fileOrDirExists(path: string): boolean;
// FileInfo ou undefined si erreur





export declare function stat(path: string): FileInfo | undefined;
// Dossier vide requis





export declare function remove(path: string): boolean;
// Dossier non vide : suppression récursive





export declare function rmrf(path: string): boolean;








export declare function rename(oldPath: string, newPath: string): boolean;
// Copie dossier récursive, même non vide






export declare function copy(oldPath: string, newPath: string): boolean;





export declare function fsInfo(filesystem: string): FsInfo | undefined;
// Renvoie basename avec suffixe numérique disponible, sans extension/répertoire
// maxLen borne nom avec suffixe

















export declare function nextAvailableFilename(dirPath: string, fileName: string, fileExt: string, maxLen: number): string;

// Opérations chemins sans accès filesystem

// Équivalence selon règles filesystem




export declare function arePathsEqual(path1: string, path2: string): boolean;
// Sous-chemin selon règles équivalence filesystem






export declare function isSubpathOf(parentPath: string, childPath: string): boolean;
