#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include "bstr.h"
#include "blog.h"
#include "hiredis_helper.h"



#define EXECN_SENDER	"redissms_sender"
#define EXECN_RECEIVER	"redissms_receiver"

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



#define KEY_OUTBOX	"redissms:out"

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

		ret = send_sms(msg);
		if(ret != 0) {
			blogf("Couldn't send SMS.");
			break;
		}

		buninit(&msg);
	}

}


#define OUT_DIR	"/var/spool/sms/outgoing"

int
send_sms(bstr_t *msg)
{
	if(bstrempty(msg))
		return EINVAL;


	return 0;
}

