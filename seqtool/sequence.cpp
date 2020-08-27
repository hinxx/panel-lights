#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

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

Sequence loadSequence(const FileName *fileName)
{
    fprintf(stderr, "User supplied file name: '%s'\n", fileName->name);

//    sequence.loadFromFile(fileName);
    char buf[128];
    sprintf(buf, "%s/%s", fileName->path, fileName->name);
    fprintf(stderr, "opening %s ..\n", buf);
    FILE *fp = fopen(buf, "r");
    assert(fp != NULL);
    if (fp == NULL) {
        fprintf(stderr, "fopen() failed: %d %s\n", errno, strerror(errno));
        return Sequence();
    }

    char *p;
    bool hasShortName = false;
    // make short name same as filename for now; will be replaced if short name
    // is found in the file while parsing
    Sequence sequence(fileName->name, fileName->name);

    do {
        p = fgets(buf, 128, fp);
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

        // nread will hold the newline char as well
        int nread = strlen(buf);
        fprintf(stderr, "line: nbytes %d: '%s'\n", nread, buf);
        // empty line??! ; this can happen here (fgets() will set p to NULL above and that is handled)
//        if (nread == 0) {
//            continue;
//        }
        // line with newline only?
        if ((buf[0] == '\n') || ((nread > 1) && (buf[0] == '\r') && (buf[1] == '\n'))) {
            continue;
        }
        // check if this is a comment line
        if (buf[0] == '#') {
            // first commented line is short sequence name
            if (hasShortName == false) {
                char *s = buf;
                char *e = &buf[nread - 1];
                while (e > s) {
                    if (! isspace(*e)) break;
                    e--;
                }
                *(e + 1) = '\0';
                while (s < e) {
                    if ((! isblank(*s)) && ! (*s == '#')) break;
                    s++;
                }
                fprintf(stderr, "Extracted short name: '%s'\n", s);
                sequence.setShortName(s);
                hasShortName = true;
            } else {
                // other commented lines, before first data lines, are considered description
                char *s = buf;
                char *e = &buf[nread - 1];
                while (e > s) {
                    if (! isspace(*e)) break;
                    e--;
                }
                *(e + 1) = '\0';
                while (s <= e) {
                    if ((! isblank(*s)) && ! (*s == '#')) break;
                    s++;
                }
                fprintf(stderr, "Extracted description: '%s'\n", s);
                // skip empty lines
                if (strlen(s) > 0) {
                    sequence.appendDescription(s);
                }
            }

        } else {
            unsigned char mode1;
            unsigned int color1;
            unsigned char mode2;
            unsigned int color2;
            unsigned char wait;
            int nconv = sscanf(buf, "%hhX %X %hhX %X %hhu", &mode1, &color1, &mode2, &color2, &wait);
            if (nconv != 5) {
                fprintf(stderr, "data line invalid! nconv %d, buf: '%s'\n", nconv, buf);
                assert(nconv == 5);
            }
            sequence.addStep(Step(mode1, color1, mode2, color2, wait));
        }

    } while (!feof(fp));

    fclose(fp);

    // calculate complete sequence duration
    sequence.calcDuration();
    // mark sequence as usable by ui
    sequence.valid = true;
    fprintf(stderr, "sequence %s duration %f s\n", sequence.getShortName(), sequence.duration);
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
