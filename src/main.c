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
static void check_device(char *device_path);
static char* concatenate(char *mp_path, char *data_path);

static int uid;
static char *path;
static char *dev_path;

int main(int argc, char **argv)
{
    time_t last_touch_time = 0;
    int fd;

    if(parse_args(argc, argv))
	return 1;

    check_device(dev_path);

    if((fd = open(dev_path, O_RDONLY | O_NONBLOCK)) == -1) {
	fprintf(stderr, "Failed to open device: %s, error: %s\n", dev_path, strerror(errno));
	return 1;
    }

    if(seteuid(uid) == -1) {
	perror("Set uid failed");
	uid = 0;
    }

    if(!uid) {
	const char *st = "/";
	if(strncmp(path, st, 1) != 0) {
	    fprintf(stderr, "If you run this program as root you should pass the absolute path to a valid mp3 player.\n");
	    return 1;
	}
    }

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
		char *cmd = concatenate(path, "data/dtms.mp3");
		system(cmd);
		last_touch_time = now;
	    }
	}
    }
    close(fd);
}

static int parse_args(int argc, char **argv)
{
    for(int i=1; i<argc; i++) {
	if((strcmp(argv[i], "-h") == 0)) {
	    print_help();
	    exit(0);
	}
	if((strcmp(argv[i], "-p") == 0)) {
	    if(argv[i+1]) {
		path = argv[i+1];
	    }
	    else {
		fprintf(stderr, "Invalid path. Please give the absolute path to an mp3 player.\n");
		exit(1);
	    }
	}

	if((strcmp(argv[i], "-u") == 0)) {
	    if(argv[i+1]) {
		struct passwd *passwd = getpwnam(argv[i+1]);
		if(!passwd) {
		    fprintf(stderr, "Failed to get uid for: %s : %s.\n", argv[i+1], strerror(errno));
		    exit(1);
		}
		uid = passwd->pw_uid;
	    }
	    else {
		fprintf(stderr, "Invalid username. Type -u `whoami`.\n");
		exit(1);
	    }
	}
	if((strcmp(argv[i], "-d") == 0)) {
	    if(argv[i+1]) {
		dev_path = argv[i+1];
	    }
	    else {
		fprintf(stderr, "Invalid device file.\n");
		exit(1);
	    }
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

static void check_device(char *device_path)
{
    struct stat sb;
    if(stat(dev_path, &sb) == -1) {
	perror("stat");
	exit(0);
    }
    if(((sb.st_mode & S_IFMT) != S_IFBLK) && ((sb.st_mode & S_IFMT) != S_IFCHR)) {
	fprintf(stderr, "Invalid device file.\n");
	exit(0);
    }
}

static char *concatenate(char *mp_path, char *data_path)
{
    char *res = malloc(strlen(mp_path) + 1 + strlen(data_path) + 1);
    strcpy(res, mp_path);
    strcat(res, " ");
    strcat(res, data_path);

    return res;
}
