// 3D Filled Vector Graphics
// (c) 2019 Pawel A. Hernik
// YouTube videos:
// https://youtu.be/YLf2WXjunyg
// https://youtu.be/5y28ipwQs-E

/* Screen dimension (width / height) */
#define SCR_WD  128
#define SCR_HT  160

/* 3d field dimension (width / height) */
#define WD_3D   128
#define HT_3D   128

/* Statistic character size */
#define CHARSIZEX 6
#define CHARSIZEY 8

/* Frame Buffer lines */
#define NLINES   16

/* Double buffer (if DMA transfer used -> speed inc)
   - 0 Double buffer disabled
   - 1 Double buffer enabled */
#define DOUBLEBUF 0

/* Button pin assign */
#define BUTTON    X, 0    /* If not used leave it that way */
/* Button active level (0 or 1) */
#define BUTTON_ON    0
/* Button input mode (0 = pull none, 1 = pull up, 2 = pull down) */
#define BUTTON_PU    0
