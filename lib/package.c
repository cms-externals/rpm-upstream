#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "errno.h"
#include "header.h"
#include "oldheader.h"
#include "package.h"
#include "rpmerr.h"
#include "rpmlead.h"
#include "rpmlib.h"

/* 0 = success */
/* !0 = error */
static int readOldHeader(int fd, Header * hdr, int * isSource);

/* 0 = success */
/* 1 = bad magic */
/* 2 = error */
int pkgReadHeader(int fd, Header * hdr, int * isSource) {
    struct rpmlead lead;

    if (read(fd, &lead, sizeof(lead)) != sizeof(lead)) {
	error(RPMERR_READERROR, "read failed: %s (%d)", strerror(errno), 
	      errno);
	return 2;
    }
   
    if (lead.magic[0] != RPMLEAD_MAGIC0 || lead.magic[1] != RPMLEAD_MAGIC1 ||
	lead.magic[2] != RPMLEAD_MAGIC2 || lead.magic[3] != RPMLEAD_MAGIC3) {
	error(RPMERR_NEWPACKAGE, "only packages with major numbers <= 2 are"
		" supported by this version of RPM");
	return 1;
    }

    *isSource = lead.type == RPMLEAD_SOURCE;

    if (lead.major == 1) {
	readOldHeader(fd, hdr, isSource);
    } else if (lead.major == 2) {
	*hdr = readHeader(fd);
	if (! *hdr) return 2;
    } else {
	error(RPMERR_NEWPACKAGE, "only packages with major numbers <= 2 are"
		" supported by this version of RPM");
	return 2;
    } 

    return 0;
}

static int readOldHeader(int fd, Header * hdr, int * isSource) {
    struct oldrpmHeader oldheader;
    struct oldrpmHeaderSpec spec;
    Header dbentry;
    int_32 installTime = 0;
    char ** fileList;
    char ** fileMD5List;
    char ** fileLinktoList;
    int_32 * fileSizeList;
    int_32 * fileUIDList;
    int_32 * fileGIDList;
    int_32 * fileMtimesList;
    int_32 * fileFlagsList;
    int_16 * fileModesList;
    int_16 * fileRDevsList;
    char * fileStatesList;
    int i;

    lseek(fd, 0, SEEK_SET);
    if (oldhdrReadFromStream(fd, &oldheader)) {
	return 1;
    }

    if (oldhdrParseSpec(&oldheader, &spec)) {
	return 1;
    }

    dbentry = newHeader();
    addEntry(dbentry, RPMTAG_NAME, STRING_TYPE, oldheader.name, 1);
    addEntry(dbentry, RPMTAG_VERSION, STRING_TYPE, oldheader.version, 1);
    addEntry(dbentry, RPMTAG_RELEASE, STRING_TYPE, oldheader.release, 1);
    addEntry(dbentry, RPMTAG_DESCRIPTION, STRING_TYPE, 
	     spec.description, 1);
    addEntry(dbentry, RPMTAG_BUILDTIME, INT32_TYPE, &spec.buildTime, 1);
    addEntry(dbentry, RPMTAG_BUILDHOST, STRING_TYPE, spec.buildHost, 1);
    addEntry(dbentry, RPMTAG_INSTALLTIME, INT32_TYPE, &installTime, 1); 
    addEntry(dbentry, RPMTAG_DISTRIBUTION, STRING_TYPE, 
	     spec.distribution, 1);
    addEntry(dbentry, RPMTAG_VENDOR, STRING_TYPE, spec.vendor, 1);
    addEntry(dbentry, RPMTAG_SIZE, INT32_TYPE, &oldheader.size, 1);
    addEntry(dbentry, RPMTAG_COPYRIGHT, STRING_TYPE, spec.copyright, 1); 
    addEntry(dbentry, RPMTAG_GROUP, STRING_TYPE, oldheader.group, 1);

    *hdr = dbentry;

    if (spec.fileCount) {
	/* some packages have no file lists */

	fileList = malloc(sizeof(char *) * spec.fileCount);
	fileLinktoList = malloc(sizeof(char *) * spec.fileCount);
	fileMD5List = malloc(sizeof(char *) * spec.fileCount);
	fileSizeList = malloc(sizeof(int_32) * spec.fileCount);
	fileUIDList = malloc(sizeof(int_32) * spec.fileCount);
	fileGIDList = malloc(sizeof(int_32) * spec.fileCount);
	fileMtimesList = malloc(sizeof(int_32) * spec.fileCount);
	fileFlagsList = malloc(sizeof(int_32) * spec.fileCount);
	fileModesList = malloc(sizeof(int_16) * spec.fileCount);
	fileRDevsList = malloc(sizeof(int_16) * spec.fileCount);
	fileStatesList = malloc(sizeof(char) * spec.fileCount);

	for (i = 0; i < spec.fileCount; i++) {
	    fileList[i] = spec.files[i].path;
	    fileMD5List[i] = spec.files[i].md5;
	    fileSizeList[i] = spec.files[i].size;
	    fileUIDList[i] = spec.files[i].uid;
	    fileGIDList[i] = spec.files[i].gid;
	    fileMtimesList[i] = spec.files[i].mtime;
	    fileModesList[i] = spec.files[i].mode;
	    fileRDevsList[i] = spec.files[i].rdev;
	    fileStatesList[i] = spec.files[i].state;

	    if (spec.files[i].linkto)
		fileLinktoList[i] = spec.files[i].linkto;
	    else
		fileLinktoList[i] = "";
	    
	    fileFlagsList[i] = 0;
	    if (spec.files[i].isdoc) 
		fileFlagsList[i] |= RPMFILE_DOC;
	    if (spec.files[i].isconf)
		fileFlagsList[i] |= RPMFILE_CONFIG;
	}

	addEntry(dbentry, RPMTAG_FILENAMES, STRING_ARRAY_TYPE, fileList, 
		 spec.fileCount);
	addEntry(dbentry, RPMTAG_FILELINKTOS, STRING_ARRAY_TYPE, 
		 fileLinktoList, spec.fileCount);
	addEntry(dbentry, RPMTAG_FILEMD5S, STRING_ARRAY_TYPE, fileMD5List, 
		 spec.fileCount);
	addEntry(dbentry, RPMTAG_FILESIZES, INT32_TYPE, fileSizeList, 
		 spec.fileCount);
	addEntry(dbentry, RPMTAG_FILEUIDS, INT32_TYPE, fileUIDList, 
		 spec.fileCount);
	addEntry(dbentry, RPMTAG_FILEGIDS, INT32_TYPE, fileGIDList, 
		 spec.fileCount);
	addEntry(dbentry, RPMTAG_FILEMTIMES, INT32_TYPE, fileMtimesList, 
		 spec.fileCount);
	addEntry(dbentry, RPMTAG_FILEFLAGS, INT32_TYPE, fileFlagsList, 
		 spec.fileCount);
	addEntry(dbentry, RPMTAG_FILEMODES, INT16_TYPE, fileModesList, 
		 spec.fileCount);
	addEntry(dbentry, RPMTAG_FILERDEVS, INT16_TYPE, fileRDevsList, 
		 spec.fileCount);
	addEntry(dbentry, RPMTAG_FILESTATES, INT8_TYPE, fileStatesList, 
		 spec.fileCount);

	free(fileList);
	free(fileLinktoList);
	free(fileMD5List);
	free(fileSizeList);
	free(fileUIDList);
	free(fileGIDList);
	free(fileMtimesList);
	free(fileFlagsList);
	free(fileModesList);
	free(fileRDevsList);
	free(fileStatesList);
    }

    return 0;
}
