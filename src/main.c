/*
 * DTMS Copyright (C) 2016 Eleni Maria Stea <elene.mst@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * */

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static int parse_args(int argc, char **argv);
static void print_help();
static int check_device(char *device_path);

static int uid = -1;
static char *player_path;
static char *dev_path;

#ifdef PREFIX
static const char* player_arg = PREFIX "/share/dtms/dtms.mp3";
#else
static const char* player_arg = "data/dtms.mp3";
#endif

int main(int argc, char **argv)
{
    time_t last_touch_time = 0;
    int fd;

    if(parse_args(argc, argv) == -1)
	return 1;

    if(getuid() == 0 && uid == -1) {
	fprintf(stderr, "It's a bad idea to run this program as root, either make it setuid-root, or use the -u option to specify an unprivileged user.\n");
	return 1;
    }

    if(check_device(dev_path) == -1)
	return 1;

    if((fd = open(dev_path, O_RDONLY | O_NONBLOCK)) == -1) {
	fprintf(stderr, "Failed to open device: %s, error: %s\n", dev_path, strerror(errno));
	return 1;
    }

    if(uid == -1) {
	uid = getuid();
    }

    if(seteuid(uid) == -1) {
	perror("Set uid failed");
	return 1;
    }

    if(!uid) {
	if(player_path[0] != '/') {
	    fprintf(stderr, "If you run this program as root you should pass the absolute path to a valid mp3 player.\n");
	    return 1;
	}
    }

    char *cmd = malloc(strlen(player_path) + 1 + strlen(player_arg) + 1);
    sprintf(cmd, "%s %s", player_path, player_arg);

    while(1) {
	fd_set read_set;
	FD_ZERO(&read_set);
	FD_SET(fd, &read_set);

	int res;
	while((res = select(fd + 1, &read_set, 0, 0, 0)) == -1 && errno == EINTR);
	if(res < 0) {
	    perror("Select failed");
	    break;
	}
	if(res == 0) //nothing to read
	    continue;

	if(FD_ISSET(fd, &read_set)) {
	    char buf[1024];
	    time_t now = time(0);
	    while(read(fd, buf, sizeof buf) > 0);
	    if (now - last_touch_time > 2) {
		system(cmd);
		last_touch_time = now;
	    }
	}
    }

    free(cmd);
    close(fd);

    return 0;
}

static int parse_args(int argc, char **argv)
{
    for(int i=1; i<argc; i++) {
	if((strcmp(argv[i], "-h") == 0)) {
	    print_help();
	    exit(0);
	}

	else if((strcmp(argv[i], "-p") == 0)) {
	    if(argv[++i]) {
		player_path = argv[i];
	    }
	    else {
		fprintf(stderr, "Invalid path. Please give the absolute path to an mp3 player.\n");
		return -1;
	    }
	}

	else if((strcmp(argv[i], "-u") == 0)) {
	    if(argv[++i]) {
		struct passwd *passwd = getpwnam(argv[i]);
		if(!passwd) {
		    fprintf(stderr, "Failed to get uid for: %s : %s.\n", argv[i], strerror(errno));
		    return -1;
		}
		uid = passwd->pw_uid;
		if(uid == 0) {
		    fprintf(stderr, "You should pass an unprivileged username.\n");
		    return -1;
		}
	    }
	    else {
		fprintf(stderr, "Missing username.\n");
		return -1;
	    }
	}
	else if((strcmp(argv[i], "-d") == 0)) {
	    if(argv[++i]) {
		dev_path = argv[i];
	    }
	    else {
		fprintf(stderr, "Invalid device file.\n");
		return -1;
	    }
	}
	else {
	    fprintf(stderr, "Unknown argument: %s\n", argv[i]);
	    return -1;
	}
    }
    return 0;
}

static void print_help()
{
    printf("Options:\n");
    printf("-h, prints this help\n");
    printf("-d, path to the device\n");
    printf("-p, path to the mp3 player\n");
    printf("-u. username of the user that runs the program\n");
    printf("--------\n");
    printf("Examples:\n");
    printf("--------\n");
    printf("./dtms -d /dev/usb/hiddev0 -u eleni -p /usr/bin/mpv\n");
}

static int check_device(char *device_path)
{
    struct stat sb;
    if(stat(dev_path, &sb) == -1) {
	perror("stat");
	return -1;
    }
    if(((sb.st_mode & S_IFMT) != S_IFBLK) && ((sb.st_mode & S_IFMT) != S_IFCHR)) {
	fprintf(stderr, "%s is not a device file.\n", device_path);
	return -1;
    }
    return 0;
}
