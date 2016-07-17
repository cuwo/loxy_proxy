all:
	gcc -O2 -DDEBUG main_core.c base64.c lpx_list.c lpx_args.c lpx_mem.c lpx_sd.c lpx_dbg.c lpx_mt.c lpx_init.c lpx_net.c lpx_cb.c lpx_parse.c -o proxy -lanl

