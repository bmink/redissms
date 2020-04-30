#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include "bstr.h"
#include "blog.h"
#include "hiredis_helper.h"


#define EXECN_SENDER	"redissms_sender"
#define EXECN_RECEIVER	"redissms_receiver"
#define OUT_DIR		"/var/spool/sms/outgoing"
#define TO_PHONENR	"+16504308448"
#define KEY_OUTBOX	"redissms:out"

void sender_loop(void);
int send_sms(bstr_t *);

int
main(int argc, char **argv)
{
	char	*execn;
	int	ret;

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

	if(!xstrcmp(execn, EXECN_SENDER)) {
		if(argc != 1) {
			fprintf(stderr, "No arguments should be speficied\n");
			goto end_label;
		}

		sender_loop();

	} else
	if(!xstrcmp(execn, EXECN_RECEIVER)) {
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
		ret = hiredis_blpop(KEY_OUTBOX, &msg);
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

	bprintf(filen, "%s/%u.%d", OUT_DIR, time(NULL), getpid());
	bprintf(filen_tmp, "/tmp/%u.%d", time(NULL), getpid());

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

