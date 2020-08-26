#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include "sequence.h"


static int readDirectory(const char *filePath, FileList *fileList);


FileList loadFileList(const char *filePath)
{
    fprintf(stderr, "User supplied file path: '%s'\n", filePath);

    FileList fileList;
//    int count = readDirectory(filePath, &fileList);
//    if (count > 0) {
//        fprintf(stderr, "found %d files\n", count);
//    }
    readDirectory(filePath, &fileList);
    return fileList;
}

Sequence loadSequence(const FileName fileName)
{
    fprintf(stderr, "User supplied file name: '%s'\n", fileName.name);

    Sequence sequence(fileName);
//    sequence.loadFromFile(fileName);
    char buf[128];
    sprintf(buf, "%s/%s", fileName.path, fileName.name);
    fprintf(stderr, "opening %s ..\n", buf);
    FILE *fp = fopen(buf, "r");
    assert(fp != NULL);
    if (fp == NULL) {
        fprintf(stderr, "fopen() failed: %d %s\n", errno, strerror(errno));
        return Sequence();
    }

    int nread;
    char *p;
    do {
//        nread = fread(buf, 1, 128, fp);
        p = fgets(buf, 128, fp);
//        if (nread == 0) {
        if (p == NULL) {
            if (ferror(fp)) {
                // error occured
                fprintf(stderr, "fread() failed %d %s\n", errno, strerror(errno));
                fclose(fp);
                return Sequence();
            } else if (feof(fp)) {
                // end of file
                break;
            } else {
                // XXX: ???
                assert(errno != errno);
            }
        }
        nread = strlen(buf);
        fprintf(stderr, "line: nbytes %d: '%s'\n", nread, buf);
        unsigned char mode1;
        unsigned int color1;
        unsigned char mode2;
        unsigned int color2;
        unsigned char wait;
        sscanf(buf, "%hhX %X %hhX %X %hhu", &mode1, &color1, &mode2, &color2, &wait);
        sequence.addStep(Step(mode1, color1, mode2, color2, wait));

    } while (!feof(fp));

    fclose(fp);

    sequence.valid = true;
    return sequence;
}

static int readDirectory(const char *filePath, FileList *fileList)
{
    DIR *dirp = opendir(filePath);
    assert(dirp != NULL);
    if (dirp == NULL) {
        fprintf(stderr, "opendir() failed: %d - %s\n", errno, strerror(errno));
        return -1;
    }
    struct dirent *dp;
    int count = 0;
    errno = 0;
    do {
        dp = readdir(dirp);
        assert(errno == 0);
        if (dp == NULL && errno != 0) {
            fprintf(stderr, "readdir() failed: %d - %s\n", errno, strerror(errno));
            fileList->erase();
            return -1;
        }
        if (dp == NULL) {
            // end of the directory
            break;
        }
        if ((strncmp(dp->d_name, ".", 1) != 0) && (strncmp(dp->d_name, "..", 2) != 0)) {
            fprintf(stderr, "found file '%s'\n", dp->d_name);
            fileList->add(filePath, dp->d_name);
            count++;
        }
    } while (dp);
    closedir(dirp);
    return count;
}
