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
 * @brief Device tree Parser
 * @file dtfs_tree.c
 * @author Neil Armstrong (c) 2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libdtfs.h>

void prop_content(const char *path, const void *data, size_t len, void *priv)
{
    int type, i;
    unsigned count;

    type = dtfs_get_prop_type(data, len, &count);
    switch(type)
    {
        case DTFS_PROP_SIMPLE:
            printf("| %s\n", path);
            return;
        case DTFS_PROP_STRINGS:
            printf("| %s (%d) = ", path, count);
            for (i = 0 ; i < count ; ++i)
            {
                const char *str;
                if (dtfs_string_get(data, len, i, &str) == 0)
                    printf ("\"%s\"", str);
                else {
                    fprintf(stderr, "Failed to get string %d !\n", i);
                    return;
                }
                if (i < (count -1))
                    printf(", ");
            }
            printf("\n");
            break;
        case DTFS_PROP_WORDS:
            printf("| %s (%d) = <", path, count);
            for (i = 0 ; i < count ; ++i)
            {
                uint32_t w;
                dtfs_word_get(data, len, i, &w);
                printf("0x%08X", w);
                if (i < (count -1))
                    printf(" ");
            }
            printf(">\n");
            break;
        case DTFS_PROP_BYTES: {
            const int8_t *b = data;
            printf("| %s (%d) = [", path, count);
            for (i = 0 ; i < count ; ++i)
                printf("%02x", b[i]);
            printf("]\n");
        } break;
        default:
            fprintf(stderr, "Invalid Type !\n");
    }
}

void new_sub_node(const char *path, const char *name, void *priv)
{
    int type = dtfs_check_path(path, name);


    if (type == DTFS_PATH_NODE) {
        printf("+ %s/%s\n", path, name);
        dtfs_list_node(path, name, new_sub_node, NULL);
    }
    else if (type == DTFS_PATH_PROPERTY)
        dtfs_prop_get(path, name, prop_content, NULL);
    else
        fprintf(stderr, "new_sub_node: invalid path %s/%s\n", path, name);
}

int main(int argc, char **argv)
{
    char *base_path = DTFS_DEFAULT_PATH;

    if (argc > 1 && strcmp(argv[1], "-h") == 0 ) {
        fprintf(stderr, "Usage: %s [-h] [base path]\n", argv[0]);
        return 1;
    }

    if (argc > 1)
        base_path = argv[1];
        
    if (dtfs_list_node(base_path, NULL, new_sub_node, NULL) < 0) {
        printf("dtfs_list_node Failed !\n");
        return 1;
    }

    return 0;
}
