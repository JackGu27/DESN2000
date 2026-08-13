#ifndef BUTTON_H
#define BUTTON_H

#define BTN_PLUG      1     /* S1 = P0.10 */
#define BTN_DOORBELL  2     /* S2 = P0.11 */

void         button_init(void);
unsigned int button_read(unsigned int id);   /* 1 = pressed, 0 = not */
unsigned int button_edge(unsigned int id);   /* 1 once per press (rising edge) */

#endif  
