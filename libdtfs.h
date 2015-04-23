/*
 * Copyright 2015 Neil Armstrong <superna9999@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @brief Device tree Filesystem Parser
 * @file libdtfs.h
 * @author Neil Armstrong (c) 2014
 */

#ifndef _LIBDTFS_H_
#define _LIBDTFS_H_

#define DTFS_DEFAULT_PATH   "/proc/device-tree"

#include <stdint.h>
#include <string.h>

enum
{
    DTFS_PATH_ERROR = -1,
    DTFS_PATH_NODE = 0,
    DTFS_PATH_PROPERTY,
};

enum
{
    DTFS_PROP_SIMPLE = 0,
    DTFS_PROP_STRINGS,
    DTFS_PROP_WORDS,
    DTFS_PROP_BYTES
};


/**
 * Allocate a buffer with malloc, and concatenate base and path
 * with '/' between if base does not finish with '/' or path does not start with '/'
 * @param base must be 1 character minimum
 * @param path can be NULL
 * @return NULL on error, malloc allocated buffer pointer on success
 */
static inline char * dtfs_concat_path(const char *base, const char *path)
{
    int ret;
    size_t len_base = strlen(base);
    char *str;

    if (!base || len_base < 1)
        return NULL;
    if (!path)
        return strdup(base);

#ifdef _DTFS_USE_ASPRINTF_
    if (base[len_base-1] == '/')
        ret = asprintf(&str, "%s%s", base, path);
    else
        ret = asprintf(&str, "%s/%s", base, path);

    if (ret >= 0)
        return str;

    return NULL;
#else
    (void)ret;

    if (base[len_base-1] != '/' && path[0] != '/')
        str = malloc(len_base + strlen(path) + 2);
    else
        str = malloc(len_base + strlen(path) + 1);

    if(!str)
        return NULL;

    strcpy(str, base);

    if (base[len_base-1] != '/' && path[0] != '/')
        strcat(str, "/");

    strcat(str, path);

    return str;
#endif
}

/**
 * Return property type.
 * @param data is the property data buffer
 * @param len is the buffer length
 * @param count is where the count of objects (strings, words, bytes) is stored if not NULL
 * Type is either :
 * - DTFS_PROP_SIMPLE, no need to decode
 * - DTFS_PROP_STRINGS, use dtfs_string_get to get strings
 * - DTFS_PROP_WORDS, use dtfs_word_get to get word
 * - DTFS_PROP_BYTES access directly data as byte array
 * @return -1 on error.
 */
int dtfs_get_prop_type(const void *data, size_t len, unsigned *count);

/**
 * Get word index in property data.
 * Property data must be of type DTFS_PROP_WORDS
 * @param data is the property data buffer
 * @param len is the buffer length
 * @param n is the word index, must be < count returned by dtfs_get_prop_type
 * @param word is the pointer where the word must stored in host endianness
 * @return -1 on error, 0 on success
 */
int dtfs_word_get(const void *data, size_t len, unsigned n, uint32_t *word);

/**
 * Get string index in property data.
 * Property data must be of type DTFS_PROP_STRINGS
 * @param data is the property data buffer
 * @param len is the buffer length
 * @param n is the string index, must be < count returned by dtfs_get_prop_type
 * @param str is the pointer where the string pointer must be stored, the pointer is inside the data buffer
 * @return -1 on error, 0 on success
 */
int dtfs_string_get(const void *data, size_t len, unsigned n, const char **str);

/**
 * Check and determine path type.
 * @param base is the path base, size must be >= 1
 * @param path is the path complement, can be NULL
 * @return DTFS_PATH_NODE for a Node, DTFS_PATH_PROPERTY for a Property, -1 on error
 */
int dtfs_check_path(const char *base, const char *path);

/**
 * dtfs_node_list_cb data structure
 */
struct dtfs_node_list_s {
    char ** names;      /** names pointer table, must be allocated to sizeof(char*)*max */
    unsigned max;       /** indicated the maximun entries the structure can handle */
    unsigned count;     /** count the valid entries count, must be set to 0 on init */
    unsigned missed;    /** indicated the missed entries not fitting into the max entries count */
};

/**
 * Subnode and property list callback used to populate a dtfs_node_list_s structure
 * @param path is the base property path
 * @param name is the subnode or property name
 * @param priv is a pointer to a dtfs_node_list_s structure
 */
static inline void dtfs_node_list_cb(const char *path, const char *name, void *priv)
{
    struct dtfs_node_list_s * l = priv;

    if (l && l->names)
    {
        if(l->count < l->max)
            l->names[l->count++] = strdup(name);
        else
            l->missed++;
    }
}


/**
 * List node subnodes and properties, full path must be DTFS_PATH_NODE as returned by dtfs_check_path
 * @param base is the path base, size must be >= 1
 * @param node_path is the path complement, can be NULL
 * @param new_sub_node is the callback with the subnode or subpath, path and name pointer are only valid during the callback, path is allocated with malloc() and free's after the callback.
 * @param priv private data given to the callback
 * @return -1 on error, 0 on success
 */
int dtfs_list_node(const char *base, const char *node_path, void (*new_sub_node)(const char *path, const char *name, void *priv), void *priv);

/**
 * dtfs_prop_data_cb data structure
 */
struct dtfs_prop_data_s {
    void * data;    /** data buffer pointer allocated with malloc or NULL on error */
    size_t len;     /** data buffer length or -1 on error */
};

/**
 * Property data extraction callback to be used with dtfs_prop_get
 * @param path is the full property path
 * @param data is the data buffer allocated with malloc, it will be duplicated
 * @param len is the data buffer length
 * @param priv is a pointer to a dtfs_prop_data_s structure
 */
static inline void dtfs_prop_data_cb(const char *path, const void *data, size_t len, void *priv)
{
    struct dtfs_prop_data_s * d = priv;

    if (d)
    {
        d->data = malloc(len);
        if(d->data) { 
            memcpy(d->data, data, len);
            d->len = len;
        }
        else
            d->len = -1;
    }
}

/**
 * Get property data, full path must be DTFS_PATH_PROPERTY as returned by dtfs_check_path
 * @param base is the path base, size must be >= 1
 * @param path is the path complement, can be NULL
 * @param prop_content is the callback with the data and path allocated with malloc() and free'd after the callback, it should be copied to be used out of the callback
 * @param priv private data given to the callback
 * @return -1 on error, 0 on success
 */
int dtfs_prop_get(const char *base, const char *path, void (*prop_content)(const char *path, const void *data, size_t len, void *priv), void *priv);

#endif
