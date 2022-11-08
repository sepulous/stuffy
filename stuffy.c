#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "table_entry.h"

const uint32_t TABLE_SIGNATURE = 0x01535446;

void add_file(char*, char*);
void remove_file(char*, char*);
void list_files(char*);
void extract_file(char*, char*);
int file_in_archive(char*, char*);
TableEntry** extract_table(char*, size_t*, long*);

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("stuffy: no action specified\n");
        exit(0);
    }

    char* action = argv[1];
    if (strcmp(action, "-a") == 0) // Add file to archive
    {
        if (argc < 4)
        {
            printf("stuffy: too few arguments\n");
            exit(0);
        }

        char* archive = argv[2];
        char* file = argv[3];
        add_file(archive, file);
    }
    else if (strcmp(action, "-r") == 0) // Remove file from archive
    {
        if (argc < 4)
        {
            printf("stuffy: too few arguments\n");
            exit(0);
        }

        char* archive = argv[2];
        char* file = argv[3];
        remove_file(archive, file);
    }
    else if (strcmp(action, "-l") == 0) // List files in archive
    {
        if (argc < 3)
        {
            printf("stuffy: too few arguments\n");
            exit(0);
        }

        char* archive = argv[2];
        list_files(archive);
    }
    else if (strcmp(action, "-e") == 0) // Extract file from archive
    {
        if (argc < 4)
        {
            printf("stuffy: too few arguments\n");
            exit(0);
        }

        char* archive = argv[2];
        char* file = argv[3];
        extract_file(archive, file);
    }
    else
    {
        printf("stuffy: %s: unrecognized action\n", action);
    }
}

void add_file(char* archiveName, char* fileName)
{
    if (access(fileName, F_OK) != 0)
    {
        printf("stuffy: %s: file not found\n", fileName);
        exit(0);
    }
    
    if (access(archiveName, F_OK) == 0) // Archive exists
    {
        if (file_in_archive(archiveName, fileName))
        {
            printf("stuffy: %s: already in archive; must be removed first\n", fileName, archiveName);
            exit(0);
        }

        FILE* archive = fopen(archiveName, "rb+");
        FILE* file = fopen(fileName, "rb");
        
        size_t entryCount;
        long tableOffset;
        TableEntry** table = extract_table(archiveName, &entryCount, &tableOffset);

        unsigned char buffer[128];
        fseek(archive, tableOffset, SEEK_SET);
        while (!feof(file))
        {
            int read = fread(buffer, sizeof(unsigned char), 128, file);
            fwrite(buffer, sizeof(unsigned char), read, archive);
        }

        fseek(file, SEEK_SET, SEEK_END);
        long fileSize = ftell(file);

        fwrite(&TABLE_SIGNATURE, sizeof(TABLE_SIGNATURE), 1, archive);
        for (int i = 0; i < entryCount; i++)
        {
            TableEntry* entry = table[i];
            fwrite(entry, sizeof(TableEntry), 1, archive);
        }

        TableEntry newEntry;
        strncpy(newEntry.name, fileName, sizeof(newEntry.name) - 1);
        newEntry.offset = tableOffset;
        newEntry.size = fileSize;
        fwrite(&newEntry, sizeof(TableEntry), 1, archive);

        fclose(archive);
        fclose(file);
    }
    else // Archive doesn't exist
    {
        FILE* archive = fopen(archiveName, "ab+");
        FILE* file = fopen(fileName, "rb");

        long fileSize;
        unsigned char buffer[128];
        while (!feof(file))
        {
            int read = fread(buffer, sizeof(unsigned char), 128, file);
            fwrite(buffer, sizeof(unsigned char), read, archive);
        }
        fileSize = ftell(file);

        TableEntry newEntry;
        strncpy(newEntry.name, fileName, sizeof(newEntry.name) - 1);
        newEntry.offset = 0;
        newEntry.size = fileSize;

        fwrite(&TABLE_SIGNATURE, sizeof(TABLE_SIGNATURE), 1, archive);
        fwrite(&newEntry, sizeof(TableEntry), 1, archive);

        fclose(archive);
        fclose(file);
    }
}

void remove_file(char* archiveName, char* fileName)
{
    if (access(archiveName, F_OK) == 0)
    {
        if (!file_in_archive(archiveName, fileName))
        {
            printf("stuffy: %s: file not in archive\n", fileName);
            exit(0);
        }

        char newArchiveName[strlen(archiveName) + 5];
        sprintf(newArchiveName, "tmp_%s", archiveName);

        size_t entryCount;
        TableEntry** table = extract_table(archiveName, &entryCount, NULL);
        FILE* newArchive = fopen(newArchiveName, "wb");
        FILE* oldArchive = fopen(archiveName, "rb");
        int deletedFileSize;

        // Copy non-deleted files
        for (int i = 0; i < entryCount; i++)
        {
            TableEntry* entry = table[i];
            if (strcmp(entry->name, fileName) != 0)
            {
                unsigned char buffer[entry->size];
                fseek(oldArchive, entry->offset, SEEK_SET);
                fread(buffer, sizeof(unsigned char), entry->size, oldArchive);
                fwrite(buffer, sizeof(unsigned char), entry->size, newArchive);
            }
            else
            {
                deletedFileSize = entry->size;
            }
        }

        // Write new table back
        fwrite(&TABLE_SIGNATURE, sizeof(TABLE_SIGNATURE), 1, newArchive);
        int afterDeletedFile = 0;
        for (int i = 0; i < entryCount; i++)
        {
            TableEntry* entry = table[i];
            if (strcmp(entry->name, fileName) != 0)
            {
                if (afterDeletedFile)
                    entry->offset -= deletedFileSize;

                fwrite(entry, sizeof(TableEntry), 1, newArchive);
            }
            else
            {
                afterDeletedFile = 1;
            }
        }

        fclose(oldArchive);
        remove(archiveName);
        rename(newArchiveName, archiveName);
        fclose(newArchive);
    }
    else
    {
        printf("stuffy: %s: archive not found\n", archiveName);
    }
}

void list_files(char* archiveName)
{
    if (access(archiveName, F_OK) == 0)
    {
        size_t totalSize = 0;
        size_t entryCount;
        TableEntry** table = extract_table(archiveName, &entryCount, NULL);

        printf("%s:\n", archiveName);
        for (int i = 0; i < entryCount; i++)
        {
            TableEntry* entry = table[i];
            printf("    %s (%i bytes)\n", entry->name, entry->size);
            totalSize += entry->size;
        }
        printf("Total size: %i bytes\n", totalSize);
    }
    else
    {
        printf("stuffy: %s: archive not found\n", archiveName);
    }
}

void extract_file(char* archiveName, char* fileName)
{
    if (access(archiveName, F_OK) == 0)
    {
        size_t entryCount;
        TableEntry** table = extract_table(archiveName, &entryCount, NULL);
        for (int i = 0; i < entryCount; i++)
        {
            TableEntry* entry = table[i];
            if (strcmp(entry->name, fileName) == 0)
            {
                FILE* archive = fopen(archiveName, "rb");
                fseek(archive, entry->offset, SEEK_SET);

                unsigned char buffer[entry->size];
                fread(buffer, sizeof(unsigned char), entry->size, archive);
                write(STDOUT_FILENO, buffer, entry->size);

                fclose(archive);
                return;
            }
        }

        printf("stuffy: %s: file not in archive\n", fileName);
    }
    else
    {
        printf("stuffy: %s: archive not found\n", archiveName);
    }
}

TableEntry** extract_table(char* archiveName, size_t* entryCount, long* tableOffset)
{
    FILE* archiveFile;
    TableEntry** table;
    size_t _entryCount = 0;
    size_t archiveSize;
    long _tableOffset;

    archiveFile = fopen(archiveName, "rb");
    if (archiveFile == NULL)
    {
        printf("stuffy: %s: archive not found\n", archiveName);
        exit(0);
    }

    fseek(archiveFile, SEEK_SET, SEEK_END);
    archiveSize = ftell(archiveFile);
    fseek(archiveFile, SEEK_SET, SEEK_SET);

    // Find table entries
    uint32_t buffer;
    while (ftell(archiveFile) < archiveSize)
    {
        fread(&buffer, sizeof(uint32_t), 1, archiveFile);

        if (buffer == TABLE_SIGNATURE)
            break;
        else
            fseek(archiveFile, -sizeof(uint32_t) + 1, SEEK_CUR);
    }
    _tableOffset = ftell(archiveFile) - 4;

    if (feof(archiveFile) || ferror(archiveFile))
    {
        printf("stuffy: %s: archive table not found\n", archiveName);
        exit(0);
    }

    // Determine size of table
    while (ftell(archiveFile) < archiveSize)
    {
        _entryCount++;
        fseek(archiveFile, sizeof(TableEntry), SEEK_CUR);
    }

    // Create entry array
    int tableIndex = 0;
    table = (TableEntry**)malloc(_entryCount * sizeof(TableEntry*));
    fseek(archiveFile, _tableOffset + 4, SEEK_SET);
    while (!feof(archiveFile) && !ferror(archiveFile))
    {
        TableEntry* entry = (TableEntry*)malloc(sizeof(TableEntry));
        fread(entry->name, sizeof(char), sizeof(entry->name), archiveFile);
        fread(&(entry->offset), sizeof(size_t), 1, archiveFile);
        fread(&(entry->size), sizeof(size_t), 1, archiveFile);
        table[tableIndex] = entry;
        tableIndex++;
    }

    fclose(archiveFile);

    if (entryCount != NULL)
        *entryCount = _entryCount;

    if (tableOffset != NULL)
        *tableOffset = _tableOffset;

    return table;
}

int file_in_archive(char* archiveName, char* fileName)
{
    size_t entryCount;
    TableEntry** table = extract_table(archiveName, &entryCount, NULL);

    for (int i = 0; i < entryCount; i++)
    {
        TableEntry* entry = table[i];
        if (strncmp(entry->name, fileName, strlen(fileName)) == 0)
            return 1;
    }

    return 0;
}
