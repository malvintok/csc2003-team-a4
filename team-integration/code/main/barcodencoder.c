/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "database.h"

#define BARCODE_PIN 26
#define BARCODE_THRESHOLD 1.8
#define BARCODE_INTERVAL 1
#define DICTMAXSIZE 40

const float conversion_factor = 3.3f / (1<<12);
int thickThreshold = 15;

// Dict init
char * dict[DICTMAXSIZE][2];   // Define an array of character pointers
const int m = 2;
const int n = 33;
struct repeating_timer barcodeTimer;

// F is [1] thin black [2] thin white [3] thick black [4] thin white [5] thick black
//      [6] thick white [7] thin black [8] thin white [9] thin black
// thin is 0, thick is 1
// black list is [0, 1, 1, 0, 0]
// white list is [0, 0, 1, 0]
// combined list in alternates for dictionary search is
//  [0,     0,      1,      0,      1,      1,      0,      0,      0] where
//  [black, white,  black,  white,  black,  white,  black,  white,  black]

//temp
static int charState = 0;               //0 = idle, 1 = first white detected, 2 = first black detected, 3 = start scanning
    static int blackBarOnes[10] = {0};
    static int whiteBarZeroes[10] = {0};
    static char binScanResult[10];
    static int currentColor = 0;
    static int blackCount = 0;
    static int whiteCount = -1;
    static char * barcodeResult = "\0";
    static int barcodeState = 0;

char* search(char* key)
{
    int result;
    char *nil = "key not found";
    for(int i=0; i<DICTMAXSIZE; i++)
    {
        result = strcmp(key, dict[i][0]); // Compare key with the stored dictionary
        if (result == 0)
        {
            return dict[i][1]; // Return corresponding character
        }
    }
    return "err";
}

void dictInit()
{
    /* 
        START OF BINARY STRING PAIR DEFINITION 
        Storing the defined binaries for each corresponding character
        according to the barcode Code 39 
        see here: https://en.wikipedia.org/wiki/Code_39#:~:text=for%20the%20character.-,Code%2039%20check%20digit,character%20is%20assigned%20a%20value. 
    */
    
    // Asterisk    
    char *a10 ="010010100"; char *b10 = "*";

    // Alphabets
    char *a11 ="100001001"; char *b11 = "A";
    char *a12 ="001001001"; char *b12 = "B";
    char *a13 ="101001000"; char *b13 = "C";
    char *a14 ="000011001"; char *b14 = "D";
    char *a15 ="100011000"; char *b15 = "E";
    char *a16 ="001011000"; char *b16 = "F";
    char *a17 ="000001101"; char *b17 = "G";
    char *a18 ="100001100"; char *b18 = "H";
    char *a19 ="001001100"; char *b19 = "I";
    char *a110 ="000011100"; char *b110 = "J";
    char *a111 ="100000011"; char *b111 = "K";
    char *a112 ="001000011"; char *b112 = "L";
    char *a113 ="101000010"; char *b113 = "M";
    char *a114 ="000010011"; char *b114 = "N";
    char *a115 ="100010010"; char *b115 = "O";
    char *a116 ="010010100"; char *b116 = "P";
    char *a117 ="000000111"; char *b117 = "Q";
    char *a118 ="100000110"; char *b118 = "R";
    char *a119 ="001000110"; char *b119 = "S";
    char *a120 ="000010110"; char *b120 = "T";
    char *a121 ="110000001"; char *b121 = "U";
    char *a122 ="011000001"; char *b122 = "V";
    char *a123 ="111000000"; char *b123 = "W";
    char *a124 ="010010001"; char *b124 = "X";
    char *a125 ="110010000"; char *b125 = "Y";
    char *a126 ="011010000"; char *b126 = "Z";

    // Numbers
    char *a127 ="100100001"; char *b127 = "1";
    char *a128 ="001100001"; char *b128 = "2";
    char *a129 ="101100000"; char *b129 = "3";
    char *a130 ="000110001"; char *b130 = "4";
    char *a131 ="100110000"; char *b131 = "5";
    char *a132 ="001110000"; char *b132 = "6";
    char *a133 ="000100101"; char *b133 = "7";
    char *a134 ="100100100"; char *b134 = "8";
    char *a135 ="001100100"; char *b135 = "9";
    char *a136 ="000110100"; char *b136 = "0";

    /* 
        END OF BINARY STRING PAIR DEFINITION 
    */

    /* 
        START OF 2D ARRAY ASSIGNMENT 
        Assigning the binary definition to the [0] index
        and the corresponding character to the [1] index

        Key value pair. dict{{"000110100", "0"}, {"001100100", "9"}, ... }        
    */

    dict[0][0] = a10; dict[0][1] = b10;
    dict[1][0] = a11; dict[1][1] = b11;
    dict[2][0] = a12; dict[2][1] = b12;
    dict[3][0] = a13; dict[3][1] = b13;
    dict[4][0] = a14; dict[4][1] = b14;
    dict[5][0] = a15; dict[5][1] = b15;
    dict[6][0] = a16; dict[6][1] = b16;
    dict[7][0] = a17; dict[7][1] = b17;
    dict[8][0] = a18; dict[8][1] = b18;
    dict[9][0] = a19; dict[9][1] = b19;
    dict[10][0] = a110; dict[10][1] = b110;
    dict[11][0] = a111; dict[11][1] = b111;
    dict[12][0] = a112; dict[12][1] = b112;
    dict[13][0] = a113; dict[13][1] = b113;
    dict[14][0] = a114; dict[14][1] = b114;
    dict[15][0] = a115; dict[15][1] = b115;
    dict[16][0] = a116; dict[16][1] = b116;
    dict[17][0] = a117; dict[17][1] = b117;
    dict[18][0] = a118; dict[18][1] = b118;
    dict[19][0] = a119; dict[19][1] = b119;
    dict[20][0] = a120; dict[20][1] = b120;
    dict[21][0] = a121; dict[21][1] = b121;
    dict[22][0] = a122; dict[22][1] = b122;
    dict[23][0] = a123; dict[23][1] = b123;
    dict[24][0] = a124; dict[24][1] = b124;
    dict[25][0] = a125; dict[25][1] = b125;
    dict[26][0] = a126; dict[26][1] = b126;
    dict[27][0] = a127; dict[27][1] = b127;
    dict[28][0] = a128; dict[28][1] = b128;
    dict[29][0] = a129; dict[29][1] = b129;
    dict[30][0] = a130; dict[30][1] = b130;
    dict[31][0] = a131; dict[31][1] = b131;
    dict[32][0] = a132; dict[32][1] = b132;
    dict[33][0] = a133; dict[33][1] = b133;
    dict[34][0] = a134; dict[34][1] = b134;
    dict[35][0] = a135; dict[35][1] = b135;
    dict[36][0] = a136; dict[36][1] = b136;
    
    /* END OF 2D ARRAY ASSIGNMENT  */
}
/*
void gpio_callback(uint gpio, uint32_t events)  //gpio interrupt
{
    if (gpio == R_ENCODER_PIN){                 
        if (events == GPIO_IRQ_EDGE_FALL){      //if edge fall detected, add to right notch counter from R_ENCODER_PIN
            rightNotchCount++;
        }
    }
    else if (gpio == L_ENCODER_PIN){            
        if (events == GPIO_IRQ_EDGE_FALL){      //if edge fall detected, add to left notch counter from L_ENCODER_PIN
            leftNotchCount++;
        } 
    }
}

bool calculateSpeed(struct repeating_timer *t)  //to calculate speed using notches count * (circumference / notch)
{
    leftSpeed = leftNotchCount * ((float)CIRCUMFERENCE/(float)NUM_NOTCH);
    rightSpeed = rightNotchCount * ((float)CIRCUMFERENCE/(float)NUM_NOTCH);
    printf("Right Wheel Speed: %.3f cm/s\n", rightSpeed);
    printf("Left Wheel Speed: %.3f cm/s\n", leftSpeed);
    leftNotchCount = 0;
    rightNotchCount = 0;
    return true;
}
*/

void barcodeReset(){
    charState = 0;
    memset(&blackBarOnes, 0, sizeof(blackBarOnes));
    memset(&whiteBarZeroes, 0, sizeof(whiteBarZeroes));
    memset(&binScanResult, 0, sizeof(binScanResult));
    currentColor = 0;
    blackCount = 0;
    whiteCount = -1; 

    barcodeResult = "\0";
    barcodeState = 0;
}//0 = idle, 1 = first white detected, 2 = first black detected, 3 = start scanning

bool scanBarcode(struct repeating_timer *t)
{
    // /*Variables of Character Scanning*/
    // static int charState = 0;               //0 = idle, 1 = first white detected, 2 = first black detected, 3 = start scanning
    // static int blackBarOnes[10] = {0};
    // static int whiteBarZeroes[10] = {0};
    // static char binScanResult[10];
    // static int currentColor = 0;
    // static int blackCount = 0;
    // static int whiteCount = -1;    

    /*ADC variables*/
    uint16_t result = adc_read();
    float voltage = result * conversion_factor;

    // /*Variable of Barcode Scanning*/
    // static char * barcodeResult = "\0";
    // static int barcodeState = 0;
    
    switch (charState){
    case 0:                                 //waiting to detect first white (starting state)
        if (barcodeState >= 1){             //skip first state if asterisk is found already
            currentColor = 0;
            charState = 1;
            break;
        }
        if (voltage < BARCODE_THRESHOLD){   //first white detected
            currentColor = 0;
            charState = 1;
        }
        else{                               //still detecting black
            currentColor = 1;
        }
        break;
    case 1:
        if (voltage > BARCODE_THRESHOLD){       //detected first black from first white
            currentColor = 1;
            charState = 2;
            //printf("Character starts!!!\n");
        }
        else{
            currentColor = 0;       
        }
        break;
    case 2:                                     //count no. of white to black (scanning state)
        if (blackCount < 5){                    //if more than 5 transitions, it means that a character has been detected
            if (voltage > BARCODE_THRESHOLD){   //black detected since voltage is more than set threshold
                if (currentColor == 0){         //white to black is detected
                    if (blackCount == 0){       //on first detection of black to white, we want to set the thick_threshold
                        thickThreshold = ceil(blackBarOnes[0] * 1.8);
                        //printf("Thick Threshold: %d\n", thickThreshold);
                    }
                    //printf("Black %d count: %d\n", blackCount, blackBarOnes[blackCount]);
                    blackCount++;
                    currentColor = 1;
                }
                blackBarOnes[blackCount]++;
                currentColor = 1;
            }
            else{                               //white detected since voltage is less than set threshold
                if (whiteCount == -1){
                    whiteCount = 0;
                }
                else{
                    if (currentColor > 0){      //black to white is detected
                        //printf("White %d count: %d\n", whiteCount, whiteBarZeroes[whiteCount]);
                        whiteCount++;
                        currentColor = 0; 
                    }
                    whiteBarZeroes[whiteCount]++;
                }
                currentColor = 0; 
            }
        }
        else{
            charState = 3;  //go to decoding state
        }
        break;
    case 3: //decode state
        printf("");
        int x = 0;
        int y = 0;
        for (int i = 0; i < 10; i++){   //go through the scan result for both black and white to combine it into one array
            if (i%2 == 0){
                if (blackBarOnes[x] > thickThreshold){
                    binScanResult[i] = '1';
                }
                else{
                    binScanResult[i] = '0';
                }
                blackBarOnes[x] = 0;
                x++;
            }
            else{
                if (whiteBarZeroes[y] > thickThreshold){
                    binScanResult[i] = '1';
                }
                else{
                    binScanResult[i] = '0';
                }
                whiteBarZeroes[y] = 0;
                y++;
            }                 
        }
        binScanResult[9] = '\0';
        //printf("Binary Scan Results: %s\n", binScanResult); //print what is scanned in a binary string
        blackCount = 0;
        whiteCount = -1;
        charState = 0;            

        barcodeResult = search(binScanResult);

        //barcodeState: 0 = first character *must be asterisk, 1 = detecting the actual message, 2 = detected end of barcode
        if (barcodeResult != "err") {
            //printf("Character results: %s\n", barcodeResult);

            switch (barcodeState){  
                case 0:                
                    //printf("Case 0 entered\n");               
                    if (barcodeResult == "*"){  //if * is detected, means we are at the start of the barcode, next character will be the acutal message
                        
                        barcodeState = 1;                    
                    }
                    else {
                        barcodeState = 0;
                    }
                    //printf("next barcode state: %d\n", barcodeState);
                    break;
                
                case 1:                         //actual message
                    // barcodeResult = 'a';
                    printf("Character results: %s\n", barcodeResult);
                    // <HERE DATABASE THE VALUE>
                    database->barcodeResult = barcodeResult;
                    barcodeState = 2;
                    //printf("Case 1 entered\n");
                    
                    //printf("next barcode state: %d\n", barcodeState);;              
                    break;

                case 2:                         //if 3 chars is detected, end of barcode
                    barcodeState = 0;
                    //printf("Case 2 entered\n");

                    //printf("next barcode state: %d\n", barcodeState);
                    break;

                default:
                    //printf("wtf barcodeState is more than 1?\n");
                    break;
            }
        }
        
        else {
            //reset list     
            barcodeResult = "err";       
            //printf("Error in scanning barcode\n");
            //printf("Car needs to reverse...\n");
        }
        break;
    default:
        break;
    }
    return true;
}



void init_Barcodencoder()
{
    // Dictionary Init
    dictInit();

/*
    // GPIO set up Left Encoder
    gpio_init(L_ENCODER_PIN);
    gpio_set_dir(L_ENCODER_PIN, GPIO_IN);
    gpio_pull_up(L_ENCODER_PIN);

    // GPIO set up Right Encoder
    gpio_init(R_ENCODER_PIN);
    gpio_set_dir(R_ENCODER_PIN, GPIO_IN);
    gpio_pull_up(R_ENCODER_PIN);
*/
    // Barcode ADC
    adc_init();
    adc_gpio_init(BARCODE_PIN);
    adc_select_input(0);
    adc_irq_set_enabled(BARCODE_PIN);

    // gpio_set_irq_enabled_with_callback(L_ENCODER_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    // gpio_set_irq_enabled(R_ENCODER_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BARCODE_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

/*
    struct repeating_timer timer;    
    add_repeating_timer_ms(SPEED_INTERVAL, calculateSpeed, NULL, &timer);

*/
    add_repeating_timer_ms(BARCODE_INTERVAL, scanBarcode, NULL, &barcodeTimer);
    
}
