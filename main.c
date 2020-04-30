#include <stdio.h>
#include <libgen.h>
#include "bstr.h"
#include "blog.h"



#define EXECN_SENDER	"redissms_sender"
#define EXECN_RECEIVER	"redissms_receiver"

void sender_loop(void);


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

	blog_uninit();

	return 0;
}



void
sender_loop(void)
{
}

