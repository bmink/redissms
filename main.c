#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include "bstr.h"
#include "blog.h"
#include "hiredis_helper.h"


#define EXECN_SENDER	"redissms_sender"
#define EXECN_RECEIVER	"redissms_receiver"
#define OUT_DIR		"/var/spool/sms/outgoing"
#define TO_PHONENR	"+16504308448"
#define KEY_OUTBOX	"redissms:out"
#define KEY_INBOX	"redissms:in"

void sender_loop(void);
int send_sms(bstr_t *);
int receive_sms(const char *);

int
main(int argc, char **argv)
{
	char		*execn;
	int		ret;
	struct timeval	now;

	execn = basename(argv[0]);
	if(xstrempty(execn)) {
		fprintf(stderr, "Can't get executable name\n");
		goto end_label;
	}

	ret = blog_init(execn);
	if(ret != 0) {
		fprintf(stderr, "Could not initialize logging: %s\n",
		    strerror(ret));
		goto end_label;
	}

	ret = hiredis_init();
	if(ret != 0) {
		fprintf(stderr, "Could not connect to redis\n");
		goto end_label;
	}

	gettimeofday(&now, NULL);
     	srandom((now.tv_sec * 1000) + (now.tv_usec / 1000));

	if(!xstrcmp(execn, EXECN_SENDER)) {
		if(argc != 1) {
			fprintf(stderr, "No arguments should be speficied\n");
			goto end_label;
		}

		sender_loop();

	} else
	if(!xstrcmp(execn, EXECN_RECEIVER)) {

		if(argc != 3) {
			blogf("Invalid argument count");
			goto end_label;
		}

		if(xstrcmp(argv[1], "RECEIVED"))
			goto end_label;

		ret = receive_sms(argv[2]);
		if(ret != 0) {
			blogf("Couldn't receive SMS");
			goto end_label;
		}
		

	} else {
		fprintf(stderr, "Do not recognize executable name.\n");
		goto end_label;
	}


end_label:

	hiredis_uninit();

	blog_uninit();

	return 0;
}




void
sender_loop(void)
{
	int	ret;
	bstr_t	*msg;

	msg = NULL;

	while(1) {
		ret = hiredis_blpop(KEY_OUTBOX, 0, &msg);
		if(ret != 0) {
			blogf("Error while BLPOPing");	
			break;
		}

		if(msg == NULL) {
			blogf("Invalid element returned");
			break;
		}

		if(!xstrcmp(bget(msg), "!!exit")) {
			blogf("Received exit message, quitting...");
			break;
		}

#if 0
		blogf("msg=%s", bget(msg));
#endif

		ret = send_sms(msg);
		if(ret != 0) {
			blogf("Couldn't send SMS.");
			break;
		}

		buninit(&msg);
	}

}



int
send_sms(bstr_t *msg)
{
	bstr_t	*filedata;
	bstr_t	*filen;
	bstr_t	*filen_tmp;
	int	err;
	int	ret;

	if(bstrempty(msg))
		return EINVAL;

	err = 0;
	filedata = NULL;
	filen = NULL;
	filen_tmp = NULL;

	filen = binit();
	if(filen == NULL) {
		blogf("Couldn't allocate filen");
		err = ENOMEM;
		goto end_label;
	}

	filen_tmp = binit();
	if(filen_tmp == NULL) {
		blogf("Couldn't allocate filen_tmp");
		err = ENOMEM;
		goto end_label;
	}

	bprintf(filen, "%s/%u.%d.%d", OUT_DIR, time(NULL), getpid(), random());
	bprintf(filen_tmp, "/tmp/%u.%d.%d", time(NULL), getpid(), random());

	filedata = binit();
	if(filedata == NULL) {
		blogf("Couldn't allocate filedata");
		err = ENOMEM;
		goto end_label;
	}

	bprintf(filedata, "To: %s\n\n%s", TO_PHONENR, bget(msg));

	ret = btofile(bget(filen_tmp), filedata);
	if(ret != 0) {
		blogf("Couldn't write temp file: %s", bget(filen_tmp));
		err = ret;
		goto end_label;
	}

	ret = rename(bget(filen_tmp), bget(filen));
	if(ret != 0) {
		blogf("Couldn't move file to outgoing directory: %s",
		    bget(filen));
		err = ret;
		goto end_label;
	}

end_label:

	if(err != 0 && !bstrempty(filen_tmp))
		unlink(bget(filen_tmp));

	buninit(&filedata);
	buninit(&filen);
	buninit(&filen_tmp);


	return err;
}


int
receive_sms(const char *filen)
{
	bstr_t	*msg;
	int	err;
	int	ret;

	if(xstrempty(filen))
		return EINVAL;

	err = 0;
	msg = NULL;

	msg = binit();
	if(!msg) {
		blogf("Couldn't init msg");
		err = ENOMEM;
		goto end_label;
	}

	ret = bfromfile(msg, filen);
	if(ret != 0) {
		blogf("Couldn't load msg");
		err = ret;
		goto end_label;
	}

#if 0
	blogf("msg=\n%s\n", bget(msg));
#endif

	ret = xstrstr(bget(msg), "\n\n");	
	if(ret < 0) {
		blogf("msg didn't contain body");
		err = ENOENT;
		goto end_label;
	}
	bstrchopl(msg, ret + 2);

	ret = hiredis_rpush(KEY_INBOX, msg);
	if(ret != 0) {
		blogf("Couldn't put msg into inbox");
		err = ret;
		goto end_label;
	}

end_label:

	buninit(&msg);

	return err;	
}
