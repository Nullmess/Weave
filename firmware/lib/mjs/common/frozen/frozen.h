/*
 * Copyright (c) 2004-2013 Sergey Lyubka <valenok@gmail.com>
 * Copyright (c) 2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CS_FROZEN_FROZEN_H_
#define CS_FROZEN_FROZEN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#if defined(_WIN32) && _MSC_VER < 1700
typedef int bool;
enum { false = 0, true = 1 };
#else
#include <stdbool.h>
#endif

enum json_token_type {
    JSON_TYPE_INVALID = 0, /* Zéro doit représenter INVALID */
    JSON_TYPE_STRING,
    JSON_TYPE_NUMBER,
    JSON_TYPE_TRUE,
    JSON_TYPE_FALSE,
    JSON_TYPE_NULL,
    JSON_TYPE_OBJECT_START,
    JSON_TYPE_OBJECT_END,
    JSON_TYPE_ARRAY_START,
    JSON_TYPE_ARRAY_END,

    JSON_TYPES_CNT
};

/* Token type/valeur ; format %T */

struct json_token {
    const char* ptr;
    int len;
    enum json_token_type type;
};

#define JSON_INVALID_TOKEN \
    { 0, 0, JSON_TYPE_INVALID }

#define JSON_STRING_INVALID -1
#define JSON_STRING_INCOMPLETE -2

/* Name : propriété ou index array ; NULL si absent */
/* Premier élément et fins objet/array : name NULL */
/* Fin conteneur : value contient son texte JSON complet */
typedef void (*json_walk_callback_t)(
    void* callback_data,
    const char* name,
    size_t name_len,
    const char* path,
    const struct json_token* token);

/* Renvoie octets traités ou code erreur négatif */

int json_walk(
    const char* json_string,
    int json_string_length,
    json_walk_callback_t callback,
    void* callback_data);

struct json_out {
    int (*printer)(struct json_out*, const char* str, size_t len);
    union {
        struct {
            char* buf;
            size_t size;
            size_t len;
        } buf;
        void* data;
        FILE* fp;
    } u;
};

extern int json_printer_buf(struct json_out*, const char*, size_t);
extern int json_printer_file(struct json_out*, const char*, size_t);

#define JSON_OUT_BUF(buf, len) \
    {                          \
        json_printer_buf, {    \
            { buf, len, 0 }    \
        }                      \
    }
#define JSON_OUT_FILE(fp)       \
    {                           \
        json_printer_file, {    \
            { (char*)fp, 0, 0 } \
        }                       \
    }

typedef int (*json_printf_callback_t)(struct json_out*, va_list* ap);

/* %B int : boolean ; %Q const char* : string échappée/null */
/* %.*Q int/const char* : string échappée de longueur explicite */
/* %V const char*, int : base64 ; %H int/const char* : hex */
/* %M callback peut consommer arguments supplémentaires */
/* Renvoie taille requise ; écriture bornée au buffer */

int json_printf(struct json_out*, const char* fmt, ...);
int json_vprintf(struct json_out*, const char* fmt, va_list ap);

/* Fichier créé ou tronqué */

int json_fprintf(const char* file_name, const char* fmt, ...);
int json_vfprintf(const char* file_name, const char* fmt, va_list ap);

/* String allouée avec NUL ; free requis ; NULL si erreur */

char* json_asprintf(const char* fmt, ...);
char* json_vasprintf(const char* fmt, va_list ap);

/* %M consomme array_ptr, array_size, elem_size, fmt */

int json_printf_array(struct json_out*, va_list* ap);

/* Clés non quotées admises ; ordre des clés indifférent */
/* %B int* ou char* si sizeof(bool)==sizeof(char) */
/* %Q char** : string allouée ; free requis */
/* %V char**, int* : base64 décodé, NUL, taille ; free requis */
/* %H int*, char** : hex décodé, NUL, taille ; free requis */
/* %M scanner/user_data ; %T json_token* */
/* Renvoie nombre conversions ou erreur négative */

int json_scanf(const char* str, int str_len, const char* fmt, ...);
int json_vscanf(const char* str, int str_len, const char* fmt, va_list ap);

typedef void (*json_scanner_t)(const char* str, int len, void* user_data);

/* Renvoie longueur token ou -1 si élément absent */

int json_scanf_array_elem(
    const char* s,
    int len,
    const char* path,
    int index,
    struct json_token* token);

/* Buffers peuvent se chevaucher ; renvoie taille requise hors NUL */

int json_unescape(const char* src, int slen, char* dst, int dlen);

/* Renvoie octets imprimés */

int json_escape(struct json_out* out, const char* str, size_t str_len);

/* Contenu alloué ; free requis ; NULL si erreur */

char* json_fread(const char* file_name);

/* json_fmt=NULL supprime ; clés absentes ajoutées */
/* Array sans index : append ; renvoie 1 si modification */

int json_setf(
    const char* s,
    int len,
    struct json_out* out,
    const char* json_path,
    const char* json_fmt,
    ...);

int json_vsetf(
    const char* s,
    int len,
    struct json_out* out,
    const char* json_path,
    const char* json_fmt,
    va_list ap);

/* Renvoie octets traités */

int json_prettify(const char* s, int len, struct json_out* out);

/* Erreur négative : fichier inchangé */

int json_prettify_file(const char* file_name);

/* key/val NULL admis ; tokens correspondants non remplis */
/* Handle opaque pour itération suivante ; NULL si terminé */

void* json_next_key(
    const char* s,
    int len,
    void* handle,
    const char* path,
    struct json_token* key,
    struct json_token* val);

/* Itération array : index au lieu de key */

void* json_next_elem(
    const char* s,
    int len,
    void* handle,
    const char* path,
    int* idx,
    struct json_token* val);

#ifndef JSON_MAX_PATH_LEN
#define JSON_MAX_PATH_LEN 256
#endif

#ifndef JSON_MINIMAL
#define JSON_MINIMAL 0
#endif

#ifndef JSON_ENABLE_BASE64
#define JSON_ENABLE_BASE64 !JSON_MINIMAL
#endif

#ifndef JSON_ENABLE_HEX
#define JSON_ENABLE_HEX !JSON_MINIMAL
#endif

#ifdef __cplusplus
}
#endif

#endif
