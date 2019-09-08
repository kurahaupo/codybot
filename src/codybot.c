#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "codybot.h"

const char *codybot_version_string = "0.2.8";

static const struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'V'},
	{"blinkenshell", no_argument, NULL, 'b'},
	{"debug", no_argument, NULL, 'd'},
	{"freenode", no_argument, NULL, 'f'},
	{"hostname", required_argument, NULL, 'H'},
	{"log", required_argument, NULL, 'l'},
	{"fullname", required_argument, NULL, 'N'},
	{"nick", required_argument, NULL, 'n'},
	{"localport", required_argument, NULL, 'P'},
	{"port", required_argument, NULL, 'p'},
	{"server", required_argument, NULL, 's'},
	{"trigger", required_argument, NULL, 't'},
	{NULL, 0, NULL, 0}
};
static const char *short_options = "hVdbfH:l:N:n:P:p:s:t:";

void HelpShow(void) {
	printf("Usage: codybot { -h/--help | -V/--version | -b/--blinkenshell | -f/--freenode | -d/--debug }\n");
	printf("               { -H/--hostname HOST | -l/--log FILENAME | -N/--fullname NAME | -n/--nick NICK }\n");
	printf("               { -P/--localport PORTNUM | -p/--port PORTNUM | -s/--server ADDR | -t/--trigger CHAR }\n");
}

int debug, socket_fd, ret, endmainloop, sh_disabled, sh_locked, cmd_timeout = 30;
unsigned long long fortune_total;
struct timeval tv0;
struct tm *tm0;
time_t t0;
char *log_filename;
char *buffer, *buffer_rx, *buffer_cmd, *buffer_log;
char trigger_char;
char *nick;
char *full_user_name;
char *hostname;

void Log(char *text) {
	FILE *fp = fopen(log_filename, "a+");
	if (fp == NULL) {
		fprintf(stderr, "##codybot::Log() error: Cannot open %s: %s\n", log_filename, strerror(errno));
		return;
	}

	gettimeofday(&tv0, NULL);
	t0 = (time_t)tv0.tv_sec;
	tm0 = gmtime(&t0);
	char *str = strdup(text);
	str[strlen(str)-1] = '\0';
	sprintf(buffer_log, "%02d:%02d:%02d.%03ld ##%s##\n", tm0->tm_hour, tm0->tm_min, tm0->tm_sec,
		tv0.tv_usec, str);
	fputs(buffer_log, fp);
	fputs(buffer_log, stdout);
	memset(buffer_log, 0, 4096);

	fclose(fp);
}

void Logr(char *text) {
	FILE *fp = fopen(log_filename, "a+");
	if (fp == NULL) {
		fprintf(stderr, "##codybot::Logr() error: Cannot open %s: %s\n", log_filename, strerror(errno));
		return;
	}

	gettimeofday(&tv0, NULL);
	t0 = (time_t)tv0.tv_sec;
	tm0 = gmtime(&t0);
	sprintf(buffer_log, "%02d:%02d:%02d.%03ld <<%s>>\n", tm0->tm_hour, tm0->tm_min, tm0->tm_sec,
		tv0.tv_usec, text);
	fputs(buffer_log, fp);
	fputs(buffer_log, stdout);
	memset(buffer_log, 0, 4096);

	fclose(fp);
}

void Logx(char *text) {
	FILE *fp = fopen(log_filename, "a+");
	if (fp == NULL) {
		fprintf(stderr, "##codybot::Logx() error: Cannot open %s: %s\n", log_filename, strerror(errno));
		return;
	}

	gettimeofday(&tv0, NULL);
	t0 = (time_t)tv0.tv_sec;
	tm0 = gmtime(&t0);
	sprintf(buffer_log, "%02d:%02d:%02d.%03ld $$%s$$\n", tm0->tm_hour, tm0->tm_min, tm0->tm_sec,
		tv0.tv_usec, text);
	fputs(buffer_log, fp);
	fputs(buffer_log, stdout);
	memset(buffer_log, 0, 4096);

	fclose(fp);
}

void Msg(char *text) {
	sprintf(buffer_log, "privmsg %s :%s\n", target, text);
	SSL_write(pSSL, buffer_log, strlen(buffer_log));
	Log(buffer_log);
	memset(buffer_log, 0, 4096);
}

void ReadCommandLoop(void) {
	char buffer_line[4096];
	while (!endmainloop) {
		memset(buffer_line, 0, 4096);
		fgets(buffer_line, 4095, stdin);
		char *cp;
		cp = buffer_line;
		if (buffer_line[0] == '\n')
			continue;
		else if (strcmp(buffer_line, "exit\n")==0 || strcmp(buffer_line, "quit\n")==0)
			endmainloop = 1;
		else if (strcmp(buffer_line, "debug on\n")==0) {
			debug = 1;
			Msg("debug = 1");
		}
		else if (strcmp(buffer_line, "debug off\n")==0) {
			debug = 0;
			Msg("debug = 0");
		}
		else if (strcmp(buffer_line, "sh_disable\n")==0) {
			sh_disabled = 1;
			Msg("sh_disabled = 1");
		}
		else if (strcmp(buffer_line, "sh_enable\n")==0) {
			sh_disabled = 0;
			Msg("sh_disabled = 1");
		}
		else if (strcmp(buffer_line, "sh_lock\n")==0) {
			sh_locked = 1;
			Msg("sh_locked = 1");
		}
		else if (strcmp(buffer_line, "sh_unlock\n")==0) {
			sh_locked = 0;
			Msg("sh_locked = 0");
		}
		else if (buffer_line[0]=='i' && buffer_line[1]=='d' && buffer_line[2]==' ') {
			char *cp;
			cp = buffer_line+3;
			char pass[1024];
			unsigned int cnt = 0;
			while (1) {
				pass[cnt] = *cp;

				++cnt;
				++cp;
				if (*cp == '\n' || *cp == '\0')
					break;
			}
			pass[cnt] = '\0';

			sprintf(buffer_cmd, "privmsg nickserv :identify %s\n", pass);
			SSL_write(pSSL, buffer_cmd, strlen(buffer_cmd));
			memset(buffer_cmd, 0, 4096);
		}
		else if (*cp=='t'&&*(cp+1)=='i'&&*(cp+2)=='m'&&*(cp+3)=='e'&&*(cp+4)=='o'&&*(cp+5)=='u'&&*(cp+6)=='t'&&*(cp+7)=='\n') {
			sprintf(buffer, "timeout = %d", cmd_timeout);
			Msg(buffer);
		}
		else if (*cp=='t'&&*(cp+1)=='i'&&*(cp+2)=='m'&&*(cp+3)=='e'&&*(cp+4)=='o'&&*(cp+5)=='u'&&*(cp+6)=='t'&&*(cp+7)==' ') {
			char str[1024];
			memset(str, 0, 1024);
			unsigned int cnt = 0;
			cp += 8;
			while (1) {
				if (*cp=='\n' || *cp=='\0') {
					str[cnt] = '\0';
					break;
				}
				else {
					str[cnt] = *cp;
					++cp;
					++cnt;
				}
			}
			cmd_timeout = atoi(str);
			if (cmd_timeout == 0)
				cmd_timeout = 10;
			sprintf(buffer, "timeout = %d\n", cmd_timeout);
			Msg(buffer);
		}
		else if (*cp=='t'&&*(cp+1)=='r'&&*(cp+2)=='i'&&*(cp+3)=='g'&&*(cp+4)=='g'&&*(cp+5)=='e'&&*(cp+6)=='r'&&*(cp+7)==' ') {
			if (*(cp+8)!='\n')
				trigger_char = *(cp+8);
			sprintf(buffer, "trigger_char = %c", trigger_char);
			Msg(buffer);
		}
		else {
			SSL_write(pSSL, buffer_line, strlen(buffer_line));
			memset(buffer_line, 0, 4096);
		}
	}
}

void SignalFunc(int signum) {
	close(socket_fd);
}

int main(int argc, char **argv) {
	signal(SIGINT, SignalFunc);
	int c;
	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c == -1)
			break;
		switch (c) {
		case 'h':
			HelpShow();
			exit(0);
		case 'V':
			printf("codybot %s\n", codybot_version_string);
			exit(0);
		case 'd':
			debug = 1;
			break;
		case 'b':
			server_ip = server_ip_blinkenshell;
			break;
		case 'f':
			server_ip = server_ip_freenode;
			break;
		case 'H':
			hostname = (char *)malloc(strlen(optarg)+1);
			sprintf(hostname, optarg);
			break;
		case 'l':
			log_filename = (char *)malloc(strlen(optarg)+1);
			sprintf(log_filename, "%s", optarg);
			break;
		case 'N':
			full_user_name = (char *)malloc(strlen(optarg)+1);
			sprintf(full_user_name, optarg);
			break;
		case 'n':
			nick = (char *)malloc(strlen(optarg)+1);
			sprintf(nick, "%s", optarg);
			break;
		case 'P':
			local_port = atoi(optarg);
			break;
		case 'p':
			server_port = atoi(optarg);
			break;
		case 's':
			ServerGetIP(optarg);
			break;
		case 't':
			trigger_char = *optarg;
			break;
		default:
			fprintf(stderr, "##codybot::main() error: Unknown argument: %c/%d\n", (char)c, c);
			break;
		}
	}

	if (!full_user_name) {
		char *name = getlogin();
		full_user_name = (char *)malloc(strlen(name)+1);
		sprintf(full_user_name, name);
	}
	if (!hostname) {
		hostname = (char *)malloc(1024);
		gethostname(hostname, 1023);
	}
	if (!local_port)
		local_port = 16384;
	if (!log_filename) {
		log_filename = (char *)malloc(strlen("codybot.log")+1);
		sprintf(log_filename, "codybot.log");
	}
	if (!trigger_char)
		trigger_char = '^';
	if (!nick) {
		nick = (char *)malloc(strlen("codybot")+1);
		sprintf(nick, "codybot");
	}
	if (!server_port)
		server_port = 6697;
	if (!server_ip)
		server_ip = server_ip_freenode;

	raw.nick = (char *)malloc(1024);
	raw.username = (char *)malloc(1024);
	raw.host = (char *)malloc(1024);
	raw.command = (char *)malloc(1024);
	raw.channel = (char *)malloc(1024);
	raw.text = (char *)malloc(4096);

	buffer_rx = (char *)malloc(4096);
	memset(buffer_rx, 0, 4096);
	buffer_cmd = (char *)malloc(4096);
	memset(buffer_cmd, 0, 4096);
	buffer_log = (char *)malloc(4096);
	memset(buffer_log, 0, 4096);
	buffer = (char *)malloc(4096);
	memset(buffer, 0, 4096);

	struct stat st;
	if(stat("sh_locked", &st)==0)
		sh_locked = 1;
	
	printf("##sh_locked: %d\n", sh_locked);
	
	FILE *fp = fopen("stats", "r");
	if (fp == NULL) {
		fprintf(stderr, "##codybot::main() error: Cannot open stats file\n");
	}
	else {
		char str[1024];
		fgets(str, 1023, fp);
		fclose(fp);
		fortune_total = atoi(str);
	}
	if (debug)
		printf("##fortune_total: %llu\n", fortune_total);

	ServerConnect();
	ThreadRXStart();
	ReadCommandLoop();
	ServerClose();

	return 0;
}
