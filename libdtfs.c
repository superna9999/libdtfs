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

/* @brief Device tree Filesystem Parser
 * @file libdtfs.c
 * @author Neil Armstrong (c) 2015
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "libdtfs.h"

static unsigned is_printable_string(const void *data, int len)
{
	const char *s = data;
	const char *ss, *se;

	/* zero length is not */
	if (len == 0)
		return 0;

	/* must terminate with zero */
	if (s[len - 1] != '\0')
		return 0;

	se = s + len;

	while (s < se) {
		ss = s;
		while (s < se && *s && isprint((unsigned char)*s))
			s++;

		/* not zero, or not done yet */
		if (*s != '\0' || s == ss)
			return 0;

		s++;
	}

	return 1;
}

int dtfs_get_prop_type(const void *data, size_t len, unsigned *count)
{
    const char *d = data;

    if (len == 0)
        return DTFS_PROP_SIMPLE;

    if (is_printable_string(data, len)) {
        if (count) {
            const char *s = d;
            *count = 0;
            do {
                s += strlen(s) + 1;
                *count += 1;
            } while (s < (d + len));
        }
        return DTFS_PROP_STRINGS;
    }
    else if ((len % 4) == 0) {
        if (count)
            *count = len/4;
        return DTFS_PROP_WORDS;
    }

    if (count)
        *count = len;
    return DTFS_PROP_BYTES;
}

int dtfs_word_get(const void *data, size_t len, unsigned n, uint32_t *word)
{
    const uint32_t *w = data;

    if (!word)
        return -1;

    if (dtfs_get_prop_type(data, len, NULL) != DTFS_PROP_WORDS)
        return -1;

    if ((n * sizeof(uint32_t)) > len)
        return -1;

    *word = ntohl(w[n]);

    return 0;
}

int dtfs_string_get(const void *data, size_t len, unsigned n, const char **str)
{
    unsigned cur;
    const char *s = data;
    const char *d = data;

    if (!str)
        return -1;

    if (dtfs_get_prop_type(data, len, NULL) != DTFS_PROP_STRINGS)
        return -1;

    cur = 0;
    do {
        if (cur == n) {
            *str = s;
            return 0;
        }
        s += strlen(s) + 1;
        cur++;
    } while (s < (d + len));

    return -1;
}

int dtfs_check_path(const char *base, const char *path)
{
    char *str;
    struct stat s;
    
    if (!base)
        return -1;

    str = dtfs_concat_path(base, path);
    if (!str) {
        fprintf(stderr, "dtfs_check_path: path concat failed (%m)\n");
        return -1;
    }

    if (stat(str, &s) != 0) {
        fprintf(stderr, "dtfs_check_path: stat failed for %s (%m)\n", str);
        free(str);
        return -1;
    }

    free(str);

    if (S_ISDIR(s.st_mode))
        return DTFS_PATH_NODE;

    if (S_ISREG(s.st_mode))
        return DTFS_PATH_PROPERTY;

    return DTFS_PATH_ERROR;
}

int dtfs_list_node(const char *base, const char *node_path, void (*new_sub_node)(const char *path, const char *name, void *priv), void *priv)
{
    char *str;
    DIR *dp;
    struct dirent *ep;

    if (new_sub_node == NULL)
        return 0;
    
    if (!base)
        return -1;

    str = dtfs_concat_path(base, node_path);
    if (!str) {
        fprintf(stderr, "dtfs_list_node: concat path failed (%m)\n");
        return -1;
    }

    dp = opendir (str);
    if (dp != NULL)
    {
        while ((ep = readdir (dp))) {
            if (ep->d_name[0] != '.')
                new_sub_node(str, ep->d_name, priv);
        }
        (void) closedir (dp);
    }
    else
        fprintf(stderr, "dtfs_list_node: Couldn't open the directory");

    free(str);

    return 0;
}


int dtfs_prop_get(const char *base, const char *path, void (*prop_content)(const char *path, const void *data, size_t len, void *priv), void *priv)
{
    char *str;
    struct stat s;
    off_t size;

    if (!prop_content)
        return 0;

    if (dtfs_check_path(base, path) != DTFS_PATH_PROPERTY)
        return -1;
    
    if (!base)
        return -1;

    str = dtfs_concat_path(base, path);
    if (!str) {
        fprintf(stderr, "dtfs_prop_get: concat path failed (%m)\n");
        return -1;
    }

    if (stat(str, &s) != 0) {
        fprintf(stderr, "dtfs_prop_get: stat failed (%m)\n");
        free(str);
        return -1;
    }

    size = s.st_size;
    if (size > 0)
    {
        FILE * f;
        void * buf;
        
        f = fopen(str, "r");
        if (!f) {
            fprintf(stderr, "dtfs_prop_get: open failed (%m)\n");
            free(str);
            return -1;
        }

        buf = malloc(size);
        if (!buf) {
            fprintf(stderr, "dtfs_prop_get: malloc buf failed (%m)\n");
            fclose(f);
            free(str);
            return -1;
        }

        if (fread(buf, 1, size, f) != size) {
            fprintf(stderr, "dtfs_prop_get: truncated read (%m)\n");
            fclose(f);
            free(buf);
            free(str);
            return -1;
        }

        fclose(f);

        prop_content(str, buf, size, priv);

        free(buf);
    }
    else
        prop_content(str, NULL, 0, priv);

    free(str);

    return 0;
}
