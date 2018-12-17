#include <stdio.h>

int _arp_cache(int argc, const char **arg)
{
    if (argc != 2) {
	printf("arp <if>\n");
	return 0;
    }

    dump_arp_table(arg[(1)]);
    
    return 0;
}

int _ifconf(int argc, const char **arg)
{
    return dump_ifstats();
}

