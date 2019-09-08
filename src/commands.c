#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <openssl/ssl.h>

#include "codybot.h"

void AsciiArt(struct raw_line *rawp) {
	FILE *fp = fopen("data-ascii.txt", "r");
	if (fp == NULL) {
		Msg("codybot::AsciiArt() error: cannot open data-ascii.txt");
		return;
	}

	fseek(fp, 0, SEEK_END);
	unsigned long filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	gettimeofday(&tv0, NULL);
	srand((unsigned int)tv0.tv_usec);
	fseek(fp, rand()%(filesize-100), SEEK_CUR);

	int c = 0, cprev, cnt = 0;
	while (1) {
		cprev = c;
		c = fgetc(fp);
		if (c == -1) {
			break;
		}
		if (cprev == '\n' && c == '%') {
			//skip the newline
			fgetc(fp);
			break;
		}
	}

	char line[1024];
	memset(line, 0, 1024);
	cnt = 0, c = ' ';
	while (1) {
        cprev = c;
        c = fgetc(fp);
        if (c == -1)
            break;
        else if (c == '%' && cprev == '\n')
            break;
		else if (c == '\n') {
			Msg(line);
			memset(line, 0, 1024);
			cnt = 0;
		}
        else
			line[cnt++] = c;
    }

	fclose(fp);
}

void Chars(struct raw_line *rawp) {
	FILE *fp = fopen("data-chars.txt", "r");
	if (fp == NULL) {
		Msg("codybot::Chars() error: Cannot open data-chars.txt: %s\n");
		return;
	}

	char chars_line[4096];
	char *str;
	while (1) {
		str = fgets(chars_line, 4095, fp);
		if (str == NULL) break;
		sprintf(buffer, "%s: %s\n", raw.nick, chars_line);
		Msg(buffer);
	}

	fclose(fp);
}

void Fortune(struct raw_line *rawp) {
	FILE *fp = fopen("data-fortunes.txt", "r");
	if (fp == NULL) {
		sprintf(buffer, "##codybot::Fortune() error: Cannot open data-fortunes.txt: %s", strerror(errno));
		Msg(buffer);
		return;
	}

	fseek(fp, 0, SEEK_END);
	unsigned long filesize = (unsigned long)ftell(fp);
	fseek(fp, 0, SEEK_SET);
	gettimeofday(&tv0, NULL);
	srand((unsigned int)tv0.tv_usec);
	fseek(fp, rand()%(filesize-500), SEEK_CUR);

	int c = 0, cprev, cnt = 0;
	char fortune_line[4096];
	memset(fortune_line, 0, 4096);
	while (1) {
		cprev = c;
		c = fgetc(fp);
		if (c == -1) {
			c = ' ';
			break;
		}
		if (cprev == '\n' && c == '%') {
			//skip the newline
			fgetc(fp);
			c = ' ';
			break;
		}
	}

	while (1) {
		cprev = c;
		c = fgetc(fp);
		if (c == -1)
			break;
		else if (c == '\t' && cprev == '\n')
			break;
		else if (c == '%' && cprev == '\n')
			break;
		else if (c == '\n' && cprev == '\n')
			fortune_line[cnt++] = ' ';
		else if (c == '\n' && cprev != '\n')
			fortune_line[cnt++] = ' ';
		else
			fortune_line[cnt++] = c;
	}

	if (strlen(fortune_line) > 0) {
		RawGetTarget(rawp);
		sprintf(buffer, "fortune: %s\n", fortune_line);
		Msg(buffer);
	
		fclose(fp);
		fp = fopen("stats", "w");
		if (fp == NULL) {
			sprintf(buffer, "codybot::Fortune() error: Cannot open stats file: %s", strerror(errno));
			Msg(buffer);
			return;
		}

		char str[1024];
		sprintf(str, "%llu\n", ++fortune_total);
		fputs(str, fp);
	}

	fclose(fp);
}

void Joke(struct raw_line *rawp) {
	FILE *fp = fopen("data-jokes.txt", "r");
	if (fp == NULL) {
		sprintf(buffer, "codybot::Joke() error: cannot open data-jokes.txt: %s", strerror(errno));
		Msg(buffer);
		return;
	}

	fseek(fp, 0, SEEK_END);
	unsigned long filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	gettimeofday(&tv0, NULL);
	srand((unsigned int)tv0.tv_usec);
	unsigned int rnd = rand()%(filesize-200);
	fseek(fp, rnd, SEEK_CUR);
	if (debug)
		printf("##filesize: %lu\n##rnd: %u\n", filesize, rnd);

	int c = 0, cprev, cnt = 0;
	while (1) {
		cprev = c;
		c = fgetc(fp);
		if (c == -1) {
			break;
		}
		if (cprev == '\n' && c == '%') {
			//skip the newline
			fgetc(fp);
			break;
		}
	}

	char joke_line[4096];
	memset(joke_line, 0, 4096);
	cnt = 0, c = ' ';
	while (1) {
        cprev = c;
        c = fgetc(fp);
        if (c == -1)
            break;
        else if (c == '\t' && cprev == '\n')
            break;
        else if (c == '%' && cprev == '\n')
            break;
        else if (c == '\n' && cprev == '\n')
            joke_line[cnt++] = ' ';
        else if (c == '\n' && cprev != '\n')
            joke_line[cnt++] = ' ';
        else
            joke_line[cnt++] = c;
    }

	RawGetTarget(rawp);
	if (strlen(joke_line) > 0) {
        sprintf(buffer, "joke: %s", joke_line);
		Msg(buffer);
    }
	else {
		Msg("codybot::Joke(): joke_line is empty!");
	}

	fclose(fp);
}

void Rainbow(struct raw_line *rawp) {
	char *colors[] = { "\003", "\00301", "\00302", "\00303", "\00304", "\00305", "\00306", "\00307", "\00308",
		"\00309", "\00310", "\00311", "\00312", "\00313", "\00314", "\00315"};
	char *cp = raw.text;

	while (*cp != ' ')
		++cp;
	++cp;
	
	char result[4096];
	memset(result, 0, 4096);
	while (1) {
		strcat(result, colors[rand()%13+2]);
		strncat(result, cp++, 1);
		if (*cp == '\0')
			break;
	}
	strcat(result, colors[0]);

	Msg(result);
}

char *slap_items[20] = {
"an USB cord", "a power cord", "a laptop", "a slice of ham", "a keyboard", "a laptop cord",
"a banana peel", "a dictionary", "an atlas book", "a biography book", "an encyclopedia",
"a rubber band", "a large trout", "a rabbit", "a lizard", "a dinosaur",
"a chair", "a mouse pad", "a C programming book", "a belt"
};
void SlapCheck(struct raw_line *rawp) {
	char *c = rawp->text;
	if ((*c==1 && *(c+1)=='A' && *(c+2)=='C' && *(c+3)=='T' && *(c+4)=='I' &&
	  *(c+5)=='O' && *(c+6)=='N' && *(c+7)==' ' &&
	  *(c+8)=='s' && *(c+9)=='l' && *(c+10)=='a' && *(c+11)=='p' && *(c+12)=='s' &&
	  *(c+13)==' ') && 
	  ((*(c+14)=='c' && *(c+15)=='o' && *(c+16)=='d' && *(c+17)=='y' &&
	  *(c+18)=='b' && *(c+19)=='o' && *(c+20)=='t' && *(c+21)==' ') ||
	  (*(c+14)=='S' && *(c+15)=='p' && *(c+16)=='r' && *(c+17)=='i' &&
	  *(c+18)=='n' && *(c+19)=='g' && *(c+20)=='S' && *(c+21)=='p' && *(c+22)=='r' &&
	  *(c+23)=='o' && *(c+24)=='c' && *(c+25)=='k' && *(c+26)=='e' && *(c+27)=='t' &&
	  *(c+28)==' '))) {
		RawGetTarget(rawp);
		gettimeofday(&tv0, NULL);
		srand((unsigned int)tv0.tv_usec);
		sprintf(buffer, "%cACTION slaps %s with %s%c", 1, rawp->nick, slap_items[rand()%20], 1);
		Msg(buffer);
	}
}

void Stats(struct raw_line *rawp) {
	FILE *fp = fopen("stats", "r");
	if (fp == NULL) {
		sprintf(buffer, "##codybot::Stats() error: Cannot open stats file: %s", strerror(errno));
		Msg(buffer);
		return;
	}
	else {
		char str[1024];
		fgets(str, 1023, fp);
		fclose(fp);
		fortune_total = atoi(str);
	}
	RawGetTarget(rawp);
	sprintf(buffer, "Given fortune cookies: %llu", fortune_total);
	Msg(buffer);
}

void Weather(struct raw_line *rawp) {
	unsigned int cnt = 0;
	char city[1024], *cp = rawp->text + strlen("^weather ");
	memset(city, 0, 1024);
	while (1) {
		if (*cp == '\n' || *cp == '\0')
			break;
		else if (cnt == 0 && *cp == ' ') {
			++cp;
			continue;
		}
		else if (*cp == ' ')
			break;
		
		city[cnt] = *cp;
		++cnt;
		++cp;
	}
	memset(rawp->text, 0, strlen(rawp->text));

	sprintf(buffer, "wget -t 1 -T 24 https://wttr.in/%s -O /tmp/weather-%s.html\n", city, city);
	system(buffer);
	sprintf(buffer,
		"sed -n \"3p\" /tmp/weather-%s.html |sed 's/_//g;s/-//g;s/\\.//g;s/`//g;s/\\\"//g;s///g;s/\\[0m//g;s/\\[38\\;5\\;[0-9][0-9][0-9]m//g;s/\\[38\\;5\\;240\\;1m//g;s@\\\\@@g;s@/@@g;s/^ *//g' > /tmp/weather-%s.temp", city, city);
	system(buffer);
	sprintf(buffer, 
		"sed -n \"4p\" /tmp/weather-%s.html |sed 's/\\[0m//g;s/\\[38\\;5\\;[0-9][0-9][0-9]m//g' |grep -o '[0-9]*' > /tmp/weather-%s.temp2", city, city);
	system(buffer);

	char temp[1024], temp2[1024];
	sprintf(buffer, "/tmp/weather-%s.temp", city);
	FILE *fp = fopen(buffer, "r");
	if (fp == NULL) {
		sprintf(buffer_cmd, "codybot::Weather() error: Cannot open %s: %s", buffer, strerror(errno));
		Msg(buffer_cmd);
		return;
	}
	fgets(temp2, 1023, fp);
	fclose(fp);
	
	// remove blanks at the start of the line
	// ie "             Partly cloudy"
	cp = temp2;
	cnt = 0;
	while (1) {
		if (*cp == '\n' || *cp == '\0')
			break;
		else if (cnt == 0 && (*cp == ' ' || *cp == '\t' || *cp == 27)) {
			++cp;
			continue;
		}
		else {
			temp[cnt] = *cp;
			++cnt;
			++cp;
		}
	}
	temp[cnt+1] = '\0';

	sprintf(buffer, "/tmp/weather-%s.temp2", city);
	fp = fopen(buffer, "r");
	if (fp == NULL) {
		sprintf(buffer_cmd, "codybot::Weather() error: Cannot open %s: %s", buffer, strerror(errno));
		Msg(buffer_cmd);
	}
	fgets(temp2, 1023, fp);
	fclose(fp);

	temp2[strlen(temp2)-1] = ' ';
	int deg_celsius = atoi(temp2);
	int deg_farenheit = (deg_celsius * 9 / 5) + 32;
	sprintf(buffer_cmd, "%s: %s %dC/%dF", city, temp, deg_celsius, deg_farenheit);
	Msg(buffer_cmd);
	
	if (!debug) {
		sprintf(buffer, "rm /tmp/weather-%s.*\n", city);
		system(buffer);
		memset(buffer, 0, 4096);
	}
}