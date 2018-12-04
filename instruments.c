#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include "instruments.h"

#define MAX_INSTRUMENTS 10
#define PATH "./patches/"

void set_instrument(INSTRUMENT * instrument, struct dirent * entry);
void read_op(OPERATOR * op, int fd);


int is_dmp(struct dirent * entry) {
	char * extension = strrchr(entry->d_name, '.');
	return !(strcmp(++extension, "dmp"));

}

int get_count(DIR * dir) {
	char * extension;
	struct dirent * entry;
	int count = 0;
	
	dir = opendir(PATH);

	while ((entry = readdir(dir)) != NULL) {
		if (is_dmp(entry)) {
			count++;
		}
	}

	rewinddir(dir);
	return count;
}

void create_instruments(PATCH_LIST ** patch_list) {
	DIR * dir = opendir(PATH);
	struct dirent * entry;
	
	int count = get_count(dir);

	if (count > MAX_INSTRUMENTS) {
		count = MAX_INSTRUMENTS;
	}

	*patch_list = malloc(sizeof(PATCH_LIST));
	(*patch_list)->count = count;
	(*patch_list)->instrument = calloc(count, sizeof(INSTRUMENT));

	for (int i = 0; i < count; i++) {
		entry = readdir(dir);
		if (is_dmp(entry) == 1) {
			set_instrument(&(*patch_list)->instrument[i], entry);
		} else {
			i--;
		}
	}	
	closedir(dir);
}

void set_instrument(INSTRUMENT * instrument, struct dirent * entry) {
	
	char path[128];
	int fd;
	uint8_t byte;

	strcpy(path, PATH);
	strcat(path, entry->d_name);
	fd = open(path, O_RDONLY);
	lseek(fd, 0, SEEK_SET);
	read(fd, &byte, 1); // read file version

	if (byte == 9) {

		read(fd, &byte, 1); // Read mode
		if (byte != 1) {
			printf("%s: invalid mode. Only FM is supported.\n", entry->d_name);
		}

		lseek(fd, 1, SEEK_CUR); // Skip an irrelevant byte for the 2612
	
	} else if (byte == 11) {
		
		read(fd, &byte, 1); // read System
		if (byte != 2) {	// If preset if for the wrong system
			printf("%s is not a Genesis/MegaDrive preset!\n", entry->d_name);
			exit(1);
		}
		
		read(fd, &byte, 1); // Read Instrument mode: FM || Standard (PSG)
		if (byte != 1) {
			printf("%s is a preset for the PSG (not yet included)\n", entry->d_name);
			exit(1);
		}

	} else {	// invalid/unsupported file version
		printf("%s has an invalid file version (%d):\n DefleMask Preset v10 & v12 are the only format supported\n", path, byte);
		exit(1);
	}

	// Same for both presets versions:
	read(fd, &instrument->fms, 1);
	read(fd, &instrument->ch1_feedback, 1);
	read(fd, &instrument->algo, 1);
	read(fd, &instrument->ams, 1);

	if (instrument->fms != 0 || instrument->ams != 0) {
		instrument->lfo_enabled = 1;
	}		

	instrument->ops_enabled = 0x0F; // DefleMask doesn't save operators on/off status
	read_op(&instrument->op1, fd);
	read_op(&instrument->op2, fd); 
	read_op(&instrument->op3, fd); 
	read_op(&instrument->op4, fd);

	close(fd);
}

void read_op(OPERATOR * op, int fd) {
	read(fd, &op->mul, 1);
	read(fd, &op->tl, 1);
	read(fd, &op->ar, 1);
	read(fd, &op->d1r, 1);
	read(fd, &op->d1l, 1);
	read(fd, &op->rr, 1);
	read(fd, &op->am, 1);
	read(fd, &op->rs, 1);
	read(fd, &op->dt1, 1);
	read(fd, &op->d2r, 1);
	read(fd, &op->ssg_eg, 1);
	op->dt1 += 1;
}
