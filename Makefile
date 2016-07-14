all:
	gcc -O2 main_core.c base64.c lpx_list.c lpx_args.c lpx_mem.c -o proxy -LDEBUG -L_GNU_SOURCE -lanl

