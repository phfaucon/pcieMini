#ifndef BOARD_UIO_H
#define BOARD_UIO_H
#include <stdint.h>

struct uio_board {
	uint32_t brd_valid;
	int     brd_num;
	int     uiofd;
	int     configfd;
	int		resfd0;
	int		resfd2;
	void* 	bar0;
	void* 	bar2;
	uint32_t sys_id;
	uint32_t timestamp;    	
};

struct int_args {
	uint32_t	arg_tag;
	void*		int_function;
	struct uio_board* p_uio_board;
};

int board_uio_open(struct uio_board* p_uio_board);
int board_uio_close(struct uio_board* p_uio_board);
int board_uio_start_int_thread(void* int_function, struct uio_board* p_uio_board);

#endif // BOARD_UIO_H
