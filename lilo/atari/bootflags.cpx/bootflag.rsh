/* GEM Resource C Source */

#include <portab.h>
#include <aes.h>
#include "BOOTFLAG.H"

#if !defined(WHITEBAK)
#define WHITEBAK    0x0040
#endif
#if !defined(DRAW3D)
#define DRAW3D      0x0080
#endif

#define FLAGS9  0x0200
#define FLAGS10 0x0400
#define FLAGS11 0x0800
#define FLAGS12 0x1000
#define FLAGS13 0x2000
#define FLAGS14 0x4000
#define FLAGS15 0x8000
#define STATE8  0x0100
#define STATE9  0x0200
#define STATE10 0x0400
#define STATE11 0x0800
#define STATE12 0x1000
#define STATE13 0x2000
#define STATE14 0x4000
#define STATE15 0x8000

OBJECT rs_object[] =
{ 
  /******** Tree 0 MAIN ****************************************************/
        -1,        1,       20, G_BOX     ,   /* Object 0  */
  NONE, NORMAL, (LONG)0x00FF1100L,
  0x0000, 0x0000, 0x0020, 0x000B,
        18,        2,       17, G_BOX     ,   /* Object 1  */
  NONE, NORMAL, (LONG)0x00FF1141L,
  0x0000, 0x0000, 0x0020, 0x0009,
         9, MP_80   , MP_NONE , G_BOX     ,   /* Object 2  */
  NONE, NORMAL, (LONG)0x00FF11F0L,
  0x0001, 0x0801, 0x000E, 0x0007,
  MP_40   ,       -1,       -1, G_BUTTON  ,   /* Object 3 MP_80 */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"0x80 GEMDOS",
  0x0001, 0x0800, 0x000C, 0x0001,
  MP_20   ,       -1,       -1, G_BUTTON  ,   /* Object 4 MP_40 */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"0x40 AtSysV",
  0x0001, 0x0801, 0x000C, 0x0001,
  MP_10   ,       -1,       -1, G_BUTTON  ,   /* Object 5 MP_20 */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"0x20 NetBSD",
  0x0001, 0x0802, 0x000C, 0x0001,
  MP_08   ,       -1,       -1, G_BUTTON  ,   /* Object 6 MP_10 */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"0x10 Linux ",
  0x0001, 0x0803, 0x000C, 0x0001,
  MP_NONE ,       -1,       -1, G_BUTTON  ,   /* Object 7 MP_08 */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"0x08 undef.",
  0x0001, 0x0804, 0x000C, 0x0001,
         2,       -1,       -1, G_BUTTON  ,   /* Object 8 MP_NONE */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"no pref.",
  0x0001, 0x0805, 0x000C, 0x0001,
        16, MT_80   , MT_NONE , G_BOX     ,   /* Object 9  */
  NONE, NORMAL, (LONG)0x00FF11F0L,
  0x0011, 0x0801, 0x000E, 0x0007,
  MT_40   ,       -1,       -1, G_BUTTON  ,   /* Object 10 MT_80 */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"0x80 GEMDOS",
  0x0001, 0x0800, 0x000C, 0x0001,
  MT_20   ,       -1,       -1, G_BUTTON  ,   /* Object 11 MT_40 */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"0x40 AtSysV",
  0x0001, 0x0801, 0x000C, 0x0001,
  MT_10   ,       -1,       -1, G_BUTTON  ,   /* Object 12 MT_20 */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"0x20 NetBSD",
  0x0001, 0x0802, 0x000C, 0x0001,
  MT_08   ,       -1,       -1, G_BUTTON  ,   /* Object 13 MT_10 */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"0x10 Linux ",
  0x0001, 0x0803, 0x000C, 0x0001,
  MT_NONE ,       -1,       -1, G_BUTTON  ,   /* Object 14 MT_08 */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"0x08 undef.",
  0x0001, 0x0804, 0x000C, 0x0001,
         9,       -1,       -1, G_BUTTON  ,   /* Object 15 MT_NONE */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"no pref.",
  0x0001, 0x0805, 0x000C, 0x0001,
        17,       -1,       -1, G_STRING  ,   /* Object 16  */
  NONE, NORMAL, (LONG)"Permanent",
  0x0001, 0x0800, 0x0009, 0x0001,
         1,       -1,       -1, G_STRING  ,   /* Object 17  */
  NONE, NORMAL, (LONG)"Temporary",
  0x0011, 0x0800, 0x0009, 0x0001,
        20, M_HELP  , M_HELP  , G_BOX     ,   /* Object 18  */
  NONE, NORMAL, (LONG)0x00FF1101L,
  0x0000, 0x0009, 0x000B, 0x0002,
        18,       -1,       -1, G_BUTTON  ,   /* Object 19 M_HELP */
  SELECTABLE|EXIT, NORMAL, (LONG)"Help",
  0x0001, 0x0800, 0x0009, 0x0001,
         0, M_OK    , M_CANCEL, G_BOX     ,   /* Object 20  */
  NONE, NORMAL, (LONG)0x00FF1101L,
  0x000B, 0x0009, 0x0015, 0x0002,
  M_CANCEL,       -1,       -1, G_BUTTON  ,   /* Object 21 M_OK */
  SELECTABLE|DEFAULT|EXIT, NORMAL, (LONG)"OK",
  0x0001, 0x0800, 0x0009, 0x0001,
        20,       -1,       -1, G_BUTTON  ,   /* Object 22 M_CANCEL */
  SELECTABLE|EXIT|LASTOB, NORMAL, (LONG)"Cancel",
  0x000B, 0x0800, 0x0009, 0x0001,
  
  /******** Tree 1 HELPT ****************************************************/
        -1, I_EXIT  ,       10, G_BOX     ,   /* Object 0  */
  NONE, NORMAL, (LONG)0x00FF1100L,
  0x0000, 0x0000, 0x0020, 0x000B,
         2,       -1,       -1, G_BUTTON  ,   /* Object 1 I_EXIT */
  SELECTABLE|DEFAULT|EXIT, NORMAL, (LONG)"Exit",
  0x0016, 0x0809, 0x0009, 0x0001,
         3,       -1,       -1, G_STRING  ,   /* Object 2  */
  NONE, NORMAL, (LONG)"The boot flag setting defines",
  0x0001, 0x0800, 0x001D, 0x0001,
         4,       -1,       -1, G_STRING  ,   /* Object 3  */
  NONE, NORMAL, (LONG)"which OS to boot.",
  0x0001, 0x0801, 0x0011, 0x0001,
         5,       -1,       -1, G_STRING  ,   /* Object 4  */
  NONE, NORMAL, (LONG)"The temporary setting is valid",
  0x0001, 0x0802, 0x001E, 0x0001,
         6,       -1,       -1, G_STRING  ,   /* Object 5  */
  NONE, NORMAL, (LONG)"only until the next power-off,",
  0x0001, 0x0803, 0x001E, 0x0001,
         7,       -1,       -1, G_STRING  ,   /* Object 6  */
  NONE, NORMAL, (LONG)"whereas the permanent setting",
  0x0001, 0x0804, 0x001D, 0x0001,
         8,       -1,       -1, G_STRING  ,   /* Object 7  */
  NONE, NORMAL, (LONG)"is kept indefinitely. The temp.",
  0x0001, 0x0805, 0x001F, 0x0001,
         9,       -1,       -1, G_STRING  ,   /* Object 8  */
  NONE, NORMAL, (LONG)"setting has precedence. The",
  0x0001, 0x0806, 0x001B, 0x0001,
        10,       -1,       -1, G_STRING  ,   /* Object 9  */
  NONE, NORMAL, (LONG)"default if \042no preference\042 is",
  0x0001, 0x0807, 0x001D, 0x0001,
         0,       -1,       -1, G_STRING  ,   /* Object 10  */
  LASTOB, NORMAL, (LONG)"selected is TOS.",
  0x0001, 0x0808, 0x0010, 0x0001
};

OBJECT *rs_trindex[] =
{ &rs_object[0],   /* Tree  0 MAIN     */
  &rs_object[23]    /* Tree  1 HELPT    */
};
