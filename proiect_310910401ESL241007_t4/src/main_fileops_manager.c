#include "db.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int manager_inventory_mode(int argc, char **argv);

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--verify") == 0) {
            const char *db = "data/inventory.db";

            for (int j = 1; j < argc; j++) {
                if (strcmp(argv[j], "--db") == 0 && j + 1 < argc) {
                    db = argv[j + 1];
                }
            }

            return verify_database(db);
        }

        if (strcmp(argv[i], "--dump") == 0) {
            const char *db = "data/inventory.db";

            for (int j = 1; j < argc; j++) {
                if (strcmp(argv[j], "--db") == 0 && j + 1 < argc) {
                    db = argv[j + 1];
                }
            }

            return dump_database(db);
        }
    }

    return manager_inventory_mode(argc, argv);
}
