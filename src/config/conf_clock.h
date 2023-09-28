#ifndef CONF_CLOCK_H_INCLUDED
#define CONF_CLOCK_H_INCLUDED

/* ===== System Clock Source Options */
#define SYSCLK_SRC_RC16MHZ    0
#define SYSCLK_SRC_RC128KHZ   1
#define SYSCLK_SRC_TRS16MHZ   2
#define SYSCLK_SRC_RC32KHZ    3
#define SYSCLK_SRC_XOC16MHZ   4
#define SYSCLK_SRC_EXTERNAL   5

/* =====  Select connected clock source */
/* #define SYSCLK_SOURCE        SYSCLK_SRC_RC128KHZ */
/* #define SYSCLK_SOURCE        SYSCLK_SRC_TRS16MHZ */
/* #define SYSCLK_SOURCE        SYSCLK_SRC_XOC16MHZ */

#define SYSCLK_SOURCE        SYSCLK_SRC_EXTERNAL

#define BOARD_EXTERNAL_CLK 3686400UL//14735600UL

/* ===== System Clock Bus Division Options */
#define CONFIG_SYSCLK_PSDIV         SYSCLK_PSDIV_4

#endif /* CONF_CLOCK_H_INCLUDED */
