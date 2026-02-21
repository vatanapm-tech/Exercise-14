#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

#define LOOP_DELAY_MS           10      // Loop sampling time (ms)
#define DEBOUNCE_TIME           40      // Debounce time (ms)
#define NROWS                   4       // Number of keypad rows
#define NCOLS                   4       // Number of keypad columns

#define ACTIVE                  0       // Keypad active state (0 = low, 1 = high)

#define NOPRESS                 '\0'    // NOPRESS character

int row_pins[] = {GPIO_NUM_3, GPIO_NUM_8, GPIO_NUM_18, GPIO_NUM_17};     // Pin numbers for rows
int col_pins[] = {GPIO_NUM_16, GPIO_NUM_15, GPIO_NUM_7, GPIO_NUM_6};   // Pin numbers for columns

char keypad_array[NROWS][NCOLS] = {   // Keypad layout
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// initialize FSM states
typedef enum {
    Wait_for_press,
    Debounce,
    Wait_for_release,
} State_t;

void init_keypad() {
    // initialize row pins as outputs and set to inactive state
    for (int i = 0; i < NROWS; i++) {                           // incrementer to count through rows
        gpio_set_direction(row_pins[i], GPIO_MODE_OUTPUT);      // set as output
        gpio_set_level(row_pins[i], !ACTIVE);                   // set high
    }

    // initialize column pins as inputs with pull-up resistors
    for (int j = 0; j < NCOLS; j++) {                           // incrementer to count through columns
        gpio_set_direction(col_pins[j], GPIO_MODE_INPUT);       // set as input
        gpio_pullup_en(col_pins[j]);                            // set internal pull up resistor
    }
}

char scan_keypad() {
    char new_key = NOPRESS;                     //declare variable to hold key that the function returns

    for (int i = 0; i < NROWS; i++) {           // scan each row, setting one row active at a time and then check each column to see if active
        gpio_set_level(row_pins[i], ACTIVE);    // set current row to active state

        for (int j = 0; j < NCOLS; j++) {                   // check each column for active state
            if (gpio_get_level(col_pins[j]) == ACTIVE) {    // if column is high,
                new_key = keypad_array[i][j];                   // get the pressed key from the array         
            }
        }
        gpio_set_level(row_pins[i], !ACTIVE);   // set current row back to inactive state
    }
    return new_key;                             // return key being pressed
}

void app_main(void) {
    init_keypad();  // initialize the keypad

    // FSM variables
    State_t state = Wait_for_press; // initial state
    char new_key = NOPRESS;         // key currently pressed
    char last_key = NOPRESS;        // last key pressed
    int time = 0;                   // time debounce delay verify
    bool timed_out = false;         // checks if key pressed lasts longer than debounce time

    while (1) {
        // update FSM inputs
        new_key = scan_keypad();                    // assign each new key press as the key that is returned from scan_keypad function 
        timed_out = (time >= DEBOUNCE_TIME);        // checks if key pressed lasts longer than debounce time 

        switch (state) {
            case Wait_for_press:
                if (new_key != NOPRESS) {           // if a key is pressed, set time = 0 and assign as new key pressed
                    time = 0;                       // reset debounce time delay incrementer
                    last_key = new_key;             // set new key pressed
                    state = Debounce;               // transition to next state
                } else {
                    state = Wait_for_press;         // if there is no new key being pressed, stay in initial state
                }
                break;

            case Debounce:
                if (timed_out) {                                // if the debounce time of 40 ms has passed and key pressed remains same, 
                    if (new_key == last_key) {                  // confirm valid key press
                        printf("Key Pressed: %c\n", last_key);  // print once whichever key is being pressed
                        state = Wait_for_release;               // transition to next state
                    } else {                                    // or else, if the key changed during debounce delay (glitch), 
                        state = Wait_for_press;                 // go back to initial state
                    }
                } else {                                        // else, keep incrementing counter until at 40 ms (debounce time), stay in state
                    time += LOOP_DELAY_MS;                      // add 10 ms to time
                }
                break;

            case Wait_for_release:
                if (new_key == NOPRESS) {      // if key released,
                    state = Wait_for_press;    // go back to waiting for press (initial state)
                } else {                       // else, if key still pressed,
                    state = Wait_for_release;  // stay in state
                }
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(LOOP_DELAY_MS)); // add loop delay
    }
}
